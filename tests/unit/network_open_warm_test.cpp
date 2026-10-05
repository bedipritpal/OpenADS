// tests/unit/network_open_warm_test.cpp
// Warm OpenTableAck (USE latency): schema + first row ride the open
// reply so a remote USE costs OpenTable + production auto-open only —
// no DescribeTable, no implicit GotoTop round-trip.
//
// Proof strategy: the in-process server accumulates per-opcode counters
// in process_mg_stats(). Snapshot them around AdsOpenTable and assert
// DescribeTable (0x4A) and GotoTop (0x40) did NOT hit the wire, then
// exercise the warmed state (immediate field read, two prefetched
// skips with no further Skip frames).

#include "doctest.h"
#include "mgmt/mg_stats.h"
#include "network/server.h"
#include "network/transport.h"
#include "network/client.h"
#include "network/wire.h"
#include "openads/ace.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path ow_tmp_dir() {
    return fs::temp_directory_path() / "openads_openwarm_test";
}

void ow_wipe() {
    std::error_code ec;
    fs::remove_all(ow_tmp_dir(), ec);
    fs::create_directories(ow_tmp_dir(), ec);
}

void seed_ow_fixture(const fs::path& dir) {
    UNSIGNED8 srv[260]{};
    std::memcpy(srv, dir.string().c_str(), dir.string().size());
    ADSHANDLE hConn = 0;
    REQUIRE(AdsConnect60(srv, ADS_LOCAL_SERVER, nullptr, nullptr, 0, &hConn) == AE_SUCCESS);

    UNSIGNED8 def[]   = "NM,C,10,0";
    UNSIGNED8 tname[] = "ow.dbf";
    ADSHANDLE hTable  = 0;
    REQUIRE(AdsCreateTable(hConn, tname, nullptr, ADS_CDX, ADS_ANSI,
                           0, 0, 0, def, &hTable) == AE_SUCCESS);
    UNSIGNED8 fld[] = "NM";
    const char* vals[] = {"alpha", "bravo", "charlie"};
    for (const char* v : vals) {
        REQUIRE(AdsAppendRecord(hTable) == AE_SUCCESS);
        UNSIGNED32 len = static_cast<UNSIGNED32>(std::strlen(v));
        REQUIRE(AdsSetString(hTable, fld,
                             reinterpret_cast<UNSIGNED8*>(const_cast<char*>(v)),
                             len) == AE_SUCCESS);
        REQUIRE(AdsWriteRecord(hTable) == AE_SUCCESS);
    }
    REQUIRE(AdsCloseTable(hTable) == AE_SUCCESS);
    REQUIRE(AdsDisconnect(hConn) == AE_SUCCESS);
}

std::uint64_t op_count(std::uint8_t op) {
    return openads::mgmt::process_mg_stats()
        .op_timing[op].count.load(std::memory_order_relaxed);
}

std::string ow_get(ADSHANDLE hTable, const char* fld) {
    UNSIGNED8 f[32]{};
    std::memcpy(f, fld, std::strlen(fld));
    UNSIGNED8 buf[64]{};
    UNSIGNED32 cap = sizeof(buf);
    REQUIRE(AdsGetField(hTable, f, buf, &cap, 0) == AE_SUCCESS);
    std::string s(reinterpret_cast<char*>(buf), cap);
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return s;
}

} // namespace

TEST_CASE("Warm open: schema + first row ride the OpenTableAck") {
    ow_wipe();
    auto dir = ow_tmp_dir();
    seed_ow_fixture(dir);

    openads::network::Server srv;
    REQUIRE(srv.start("127.0.0.1", 0).has_value());

    char uri[512]{};
    std::snprintf(uri, sizeof(uri), "tcp://127.0.0.1:%u/%s",
                  static_cast<unsigned>(srv.port()), dir.string().c_str());
    UNSIGNED8 srvbuf[512]{};
    std::memcpy(srvbuf, uri, std::strlen(uri) + 1);
    ADSHANDLE hConn = 0;
    REQUIRE(AdsConnect60(srvbuf, ADS_REMOTE_SERVER, nullptr, nullptr, 0, &hConn)
            == AE_SUCCESS);

    const std::uint64_t desc_before = op_count(0x4A);  // DescribeTable
    const std::uint64_t top_before  = op_count(0x40);  // GotoTop

    UNSIGNED8 tname[] = "ow.dbf";
    ADSHANDLE hTable  = 0;
    REQUIRE(AdsOpenTable(hConn, tname, nullptr, ADS_CDX, ADS_ANSI, ADS_SHARED,
                         ADS_COMPATIBLE_LOCKING, ADS_DEFAULT, &hTable)
            == AE_SUCCESS);

    // The open carried schema + first row: neither opcode hit the wire.
    // (Auto-open OpenIndex attempts for the missing bag still do — the
    // fixture has no .cdx — but those predate sections.)
    CHECK(op_count(0x4A) == desc_before);
    CHECK(op_count(0x40) == top_before);

    // Warmed state is immediately usable: first-row read with no fetch,
    // then two skips served from the rode-along lookahead block.
    CHECK(ow_get(hTable, "NM") == "alpha");
    const std::uint64_t skip_before = op_count(0x42);  // Skip
    REQUIRE(AdsSkip(hTable, 1) == AE_SUCCESS);
    CHECK(ow_get(hTable, "NM") == "bravo");
    REQUIRE(AdsSkip(hTable, 1) == AE_SUCCESS);
    CHECK(ow_get(hTable, "NM") == "charlie");
    CHECK(op_count(0x42) == skip_before);

    REQUIRE(AdsCloseTable(hTable) == AE_SUCCESS);
    REQUIRE(AdsDisconnect(hConn) == AE_SUCCESS);
    srv.stop();
}

TEST_CASE("Pooled re-USE: close/reopen skips OpenTable+CloseTable frames") {
    ow_wipe();
    auto dir = ow_tmp_dir();
    seed_ow_fixture(dir);

    openads::network::Server srv;
    REQUIRE(srv.start("127.0.0.1", 0).has_value());

    char uri[512]{};
    std::snprintf(uri, sizeof(uri), "tcp://127.0.0.1:%u/%s",
                  static_cast<unsigned>(srv.port()), dir.string().c_str());
    UNSIGNED8 srvbuf[512]{};
    std::memcpy(srvbuf, uri, std::strlen(uri) + 1);
    ADSHANDLE hConn = 0;
    REQUIRE(AdsConnect60(srvbuf, ADS_REMOTE_SERVER, nullptr, nullptr, 0, &hConn)
            == AE_SUCCESS);

    UNSIGNED8 tname[] = "ow.dbf";
    ADSHANDLE hTable  = 0;
    REQUIRE(AdsOpenTable(hConn, tname, nullptr, ADS_CDX, ADS_ANSI, ADS_SHARED,
                         ADS_COMPATIBLE_LOCKING, ADS_DEFAULT, &hTable)
            == AE_SUCCESS);
    REQUIRE(AdsCloseTable(hTable) == AE_SUCCESS);

    const std::uint64_t open_before  = op_count(0x20);  // OpenTable
    const std::uint64_t close_before = op_count(0x22);  // CloseTable
    const std::uint64_t top_before   = op_count(0x40);  // GotoTop

    // Re-USE within TTL: parked server handle is adopted, so neither
    // OpenTable nor the first CloseTable reached the wire. Exactly one
    // warm GotoTop repositions.
    REQUIRE(AdsOpenTable(hConn, tname, nullptr, ADS_CDX, ADS_ANSI, ADS_SHARED,
                         ADS_COMPATIBLE_LOCKING, ADS_DEFAULT, &hTable)
            == AE_SUCCESS);
    CHECK(op_count(0x20) == open_before);
    CHECK(ow_get(hTable, "NM") == "alpha");
    REQUIRE(AdsCloseTable(hTable) == AE_SUCCESS);
    CHECK(op_count(0x22) == close_before);
    CHECK(op_count(0x40) == top_before + 1);

    REQUIRE(AdsDisconnect(hConn) == AE_SUCCESS);
    srv.stop();
}

TEST_CASE("Pooled re-USE after a released lock still parks") {
    ow_wipe();
    auto dir = ow_tmp_dir();
    seed_ow_fixture(dir);

    openads::network::Server srv;
    REQUIRE(srv.start("127.0.0.1", 0).has_value());

    char uri[512]{};
    std::snprintf(uri, sizeof(uri), "tcp://127.0.0.1:%u/%s",
                  static_cast<unsigned>(srv.port()), dir.string().c_str());
    UNSIGNED8 srvbuf[512]{};
    std::memcpy(srvbuf, uri, std::strlen(uri) + 1);
    ADSHANDLE hConn = 0;
    REQUIRE(AdsConnect60(srvbuf, ADS_REMOTE_SERVER, nullptr, nullptr, 0, &hConn)
            == AE_SUCCESS);

    UNSIGNED8 tname[] = "ow.dbf";
    ADSHANDLE hTable  = 0;
    REQUIRE(AdsOpenTable(hConn, tname, nullptr, ADS_CDX, ADS_ANSI, ADS_SHARED,
                         ADS_COMPATIBLE_LOCKING, ADS_DEFAULT, &hTable)
            == AE_SUCCESS);
    REQUIRE(AdsGotoRecord(hTable, 1) == AE_SUCCESS);
    REQUIRE(AdsLockRecord(hTable, 0) == AE_SUCCESS);
    REQUIRE(AdsUnlockRecord(hTable, 0) == AE_SUCCESS);
    REQUIRE(AdsCloseTable(hTable) == AE_SUCCESS);

    // Locked then fully released: still poolable (mtfix8) -- the close
    // parks the handle and the reopen is a park hit, no wire open. Only
    // outstanding locks (locks_held) force a real close now.
    const std::uint64_t open_before = op_count(0x20);
    REQUIRE(AdsOpenTable(hConn, tname, nullptr, ADS_CDX, ADS_ANSI, ADS_SHARED,
                         ADS_COMPATIBLE_LOCKING, ADS_DEFAULT, &hTable)
            == AE_SUCCESS);
    CHECK(op_count(0x20) == open_before);
    CHECK(ow_get(hTable, "NM") == "alpha");

    REQUIRE(AdsCloseTable(hTable) == AE_SUCCESS);
    REQUIRE(AdsDisconnect(hConn) == AE_SUCCESS);
    srv.stop();
}

TEST_CASE("DropTable after pooled close really removes the file") {
    ow_wipe();
    auto dir = ow_tmp_dir();
    seed_ow_fixture(dir);

    openads::network::Server srv;
    REQUIRE(srv.start("127.0.0.1", 0).has_value());

    char uri[512]{};
    std::snprintf(uri, sizeof(uri), "tcp://127.0.0.1:%u/%s",
                  static_cast<unsigned>(srv.port()), dir.string().c_str());
    UNSIGNED8 srvbuf[512]{};
    std::memcpy(srvbuf, uri, std::strlen(uri) + 1);
    ADSHANDLE hConn = 0;
    REQUIRE(AdsConnect60(srvbuf, ADS_REMOTE_SERVER, nullptr, nullptr, 0, &hConn)
            == AE_SUCCESS);

    // Open + close parks the handle (shared, natural, never locked).
    UNSIGNED8 tname[] = "ow.dbf";
    ADSHANDLE hTable  = 0;
    REQUIRE(AdsOpenTable(hConn, tname, nullptr, ADS_CDX, ADS_ANSI, ADS_SHARED,
                         ADS_COMPATIBLE_LOCKING, ADS_DEFAULT, &hTable)
            == AE_SUCCESS);
    REQUIRE(AdsCloseTable(hTable) == AE_SUCCESS);
    {
        std::error_code ec;
        CHECK(fs::exists(dir / "ow.dbf"));
        (void)ec;
    }
    // Drop must evict the parked handle first: the file really goes.
    REQUIRE(AdsDropTable(hConn, tname, 1) == AE_SUCCESS);
    {
        std::error_code ec;
        CHECK_FALSE(fs::exists(dir / "ow.dbf", ec));
    }

    REQUIRE(AdsDisconnect(hConn) == AE_SUCCESS);
    srv.stop();
}

TEST_CASE("Open setup metadata: immutable length is returned without a frame") {
    ow_wipe(); const auto dir = ow_tmp_dir(); seed_ow_fixture(dir);
    openads::network::Server srv; REQUIRE(srv.start("127.0.0.1", 0).has_value());
    char uri[512]{}; std::snprintf(uri, sizeof(uri), "tcp://127.0.0.1:%u/%s", static_cast<unsigned>(srv.port()), dir.string().c_str());
    ADSHANDLE c = 0, t = 0; REQUIRE(AdsConnect60(reinterpret_cast<UNSIGNED8*>(uri), ADS_REMOTE_SERVER, nullptr, nullptr, 0, &c) == 0);
    UNSIGNED8 name[] = "ow.dbf";
    REQUIRE(AdsOpenTable(c, name, nullptr, ADS_CDX, ADS_ANSI, ADS_SHARED, ADS_COMPATIBLE_LOCKING, ADS_DEFAULT, &t) == 0);
    const auto before = op_count(0x6C); UNSIGNED32 len = 0;
    REQUIRE(AdsGetRecordLength(t, &len) == 0); CHECK(len == 11); CHECK(op_count(0x6C) == before);
    CHECK(ow_get(t, "NM") == "alpha");
    REQUIRE(AdsCloseTable(t) == 0); REQUIRE(AdsDisconnect(c) == 0); srv.stop();
}

TEST_CASE("Open setup metadata: old client receives no index binding section") {
    ow_wipe(); const auto dir = ow_tmp_dir(); seed_ow_fixture(dir);
    openads::network::Server srv; REQUIRE(srv.start("127.0.0.1", 0).has_value());
    auto socket = openads::network::connect_tcp("127.0.0.1", srv.port()); REQUIRE(socket.has_value());
    auto transport = openads::network::make_plain_transport(socket.value());
    openads::network::Frame connect; connect.opcode = openads::network::Opcode::Connect;
    const auto path = dir.string(); connect.payload.push_back(static_cast<std::uint8_t>(path.size())); connect.payload.push_back(static_cast<std::uint8_t>(path.size() >> 8));
    connect.payload.insert(connect.payload.end(), path.begin(), path.end()); connect.payload.insert(connect.payload.end(), 4, 0);
    REQUIRE(openads::network::write_frame(*transport, connect).has_value());
    auto reply = openads::network::read_frame(*transport); REQUIRE(reply.has_value()); REQUIRE(reply.value().opcode == openads::network::Opcode::ConnectAck);
    openads::network::Frame open; open.opcode = openads::network::Opcode::OpenTable;
    const std::string name = "ow.dbf"; open.payload.assign(name.begin(), name.end());
    REQUIRE(openads::network::write_frame(*transport, open).has_value());
    reply = openads::network::read_frame(*transport); REQUIRE(reply.has_value()); REQUIRE(reply.value().opcode == openads::network::Opcode::OpenTableAck);
    const auto& bytes = reply.value().payload; REQUIRE(bytes.size() > 6); const auto baglen = bytes[4] + 256u * bytes[5];
    const auto countpos = 6u + baglen; REQUIRE(bytes.size() > countpos); CHECK(bytes[countpos] == 2); // existing schema + row only
    transport->close(); srv.stop();
}

namespace {
class LegacyOpenTransport : public openads::network::ITransport {
    std::vector<std::uint8_t> incoming;
    std::vector<std::uint8_t> outgoing;
    bool live = true;
public:
    openads::util::Result<std::size_t> send(const std::uint8_t* data, std::size_t n) override {
        incoming.insert(incoming.end(), data, data+n); std::size_t used = 0;
        auto frame = openads::network::decode_frame(incoming.data(), incoming.size(), &used);
        if (frame) {
            openads::network::Frame reply;
            const auto op = frame.value().opcode;
            if (op == openads::network::Opcode::Hello) { reply.opcode = openads::network::Opcode::HelloAck; }
            else if (op == openads::network::Opcode::Connect) { reply.opcode = openads::network::Opcode::ConnectAck; }
            else if (op == openads::network::Opcode::OpenTable) { reply.opcode = openads::network::Opcode::OpenTableAck; reply.payload = {7,0,0,0}; }
            else { incoming.clear(); return n; }
            auto encoded = openads::network::encode_frame(reply); if (encoded) outgoing = encoded.value(); incoming.clear();
        }
        return n;
    }
    openads::util::Result<std::size_t> recv(std::uint8_t* data, std::size_t n) override {
        const auto size = std::min(n, outgoing.size()); std::copy(outgoing.begin(), outgoing.begin()+static_cast<std::ptrdiff_t>(size), data);
        outgoing.erase(outgoing.begin(), outgoing.begin()+static_cast<std::ptrdiff_t>(size)); return size;
    }
    void close() noexcept override { live = false; }
    bool valid() const noexcept override { return live; }
};
}
TEST_CASE("Open setup metadata: old server reply selects legacy fallback") {
    openads::network::RemoteConnection connection;
    REQUIRE(connection.connect_with_transport(std::make_unique<LegacyOpenTransport>(), "fixture", "", "").has_value());
    auto result = connection.open_table("fixture.dbf"); REQUIRE(result.has_value());
    CHECK(result.value().id == 7); CHECK_FALSE(result.value().has_record_length);
    CHECK_FALSE(result.value().has_schema); CHECK_FALSE(result.value().has_first_row);
    CHECK(result.value().production_index_reply.empty());
    connection.disconnect();
}
