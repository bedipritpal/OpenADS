// tests/unit/network_frame_trace_test.cpp
// wire_trace diagnostics: the optional per-frame hook sees every
// completed round trip while installed, and nothing once removed.
// Default (no hook) is the production path.
#include "doctest.h"
#include "util/log.h"
#include "network/client.h"
#include "network/server.h"
#include "openads/ace.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <string>

namespace fs = std::filesystem;

namespace {

std::atomic<unsigned> g_frames{0};
std::atomic<unsigned> g_open_frames{0};
std::atomic<unsigned> g_open_acks{0};

void count_frame(const void* conn, std::uint8_t op, std::uint32_t /*tid*/,
                 std::size_t /*req_bytes*/, std::uint8_t rep_op,
                 std::size_t /*rep_bytes*/, long long us) {
    CHECK(conn != nullptr);
    CHECK(us >= 0);
    g_frames.fetch_add(1);
    if (op == 0x20) {
        g_open_frames.fetch_add(1);
        if (rep_op == 0x21) g_open_acks.fetch_add(1);
    }
}

} // namespace

TEST_CASE("wire trace: frame hook counts round trips only while installed") {
    struct OptIn {
        bool old = openads::util::logging_enabled();
        OptIn() { openads::util::set_logging_enabled(true); }
        ~OptIn() { openads::util::set_logging_enabled(old); }
    } optin;
    auto dir = fs::temp_directory_path() / "openads_frametrace";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);

    // Seed one table locally.
    UNSIGNED8 srv[512]{};
    std::memcpy(srv, dir.string().c_str(), dir.string().size());
    ADSHANDLE hC0 = 0;
    REQUIRE(AdsConnect60(srv, ADS_LOCAL_SERVER, nullptr, nullptr, 0, &hC0)
            == AE_SUCCESS);
    UNSIGNED8 def[] = "ID,N,8,0";
    UNSIGNED8 tn0[] = "FT.DBF";
    ADSHANDLE hT0 = 0;
    REQUIRE(AdsCreateTable(hC0, tn0, nullptr, ADS_CDX, 0, 0, 0, 0, def, &hT0)
            == AE_SUCCESS);
    REQUIRE(AdsCloseTable(hT0) == AE_SUCCESS);
    REQUIRE(AdsDisconnect(hC0) == AE_SUCCESS);

    openads::network::Server s;
    REQUIRE(s.start("127.0.0.1", 0).has_value());
    char uri[512]{};
    std::snprintf(uri, sizeof(uri), "tcp://127.0.0.1:%u/%s",
                  static_cast<unsigned>(s.port()), dir.string().c_str());
    UNSIGNED8 sb[512]{};
    std::memcpy(sb, uri, std::strlen(uri) + 1);
    ADSHANDLE hC = 0;
    REQUIRE(AdsConnect60(sb, ADS_REMOTE_SERVER, nullptr, nullptr, 0, &hC)
            == AE_SUCCESS);

    openads::network::set_frame_trace_hook(&count_frame);
    UNSIGNED8 tn[] = "FT.DBF";
    ADSHANDLE hT = 0;
    REQUIRE(AdsOpenTable(hC, tn, nullptr, ADS_CDX, 0, 0, 0, 0, &hT)
            == AE_SUCCESS);
    openads::network::set_frame_trace_hook(nullptr);

    CHECK(g_frames.load() >= 1u);
    CHECK(g_open_frames.load() == 1u);
    CHECK(g_open_acks.load() == 1u);

    // Removed: further traffic is not reported.
    const unsigned seen = g_frames.load();
    REQUIRE(AdsGotoBottom(hT) == AE_SUCCESS);
    REQUIRE(AdsCloseTable(hT) == AE_SUCCESS);
    CHECK(g_frames.load() == seen);

    REQUIRE(AdsDisconnect(hC) == AE_SUCCESS);
    s.stop();
    fs::remove_all(dir, ec);
}

TEST_CASE("master diagnostics file gating and table masking integration" * doctest::skip()) {
    using openads::util::set_logging_enabled;
    CHECK_FALSE(openads::util::logging_enabled());
    const bool before = openads::util::logging_enabled();
    struct Restore { bool old; ~Restore() { set_logging_enabled(old); } } restore{before};
    auto dir = fs::temp_directory_path() / "openads_master_diag";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);
    const auto trace = (dir / "wire.log").string();
    const auto create = (dir / "create.log").string();
#if defined(_WIN32)
    _putenv_s("OPENADS_WIRE_TRACE", "1");
    _putenv_s("OPENADS_WIRE_TRACE_FILE", trace.c_str());
    _putenv_s("OPENADS_CREATE_TABLE_DIAG_FILE", create.c_str());
#else
    setenv("OPENADS_WIRE_TRACE", "1", 1);
    setenv("OPENADS_WIRE_TRACE_FILE", trace.c_str(), 1);
    setenv("OPENADS_CREATE_TABLE_DIAG_FILE", create.c_str(), 1);
#endif
    set_logging_enabled(false);
    std::string root = dir.string();
    ADSHANDLE local = 0, table = 0;
    REQUIRE(AdsConnect60(reinterpret_cast<UNSIGNED8*>(root.data()), ADS_LOCAL_SERVER,
                          nullptr, nullptr, 0, &local) == 0);
    UNSIGNED8 name[] = "SECRET_REAL_TABLE.DBF";
    UNSIGNED8 fields[] = "PRIVATE_FIELD,N,8,0";
    REQUIRE(AdsCreateTable(local, name, nullptr, ADS_CDX, 0, 0, 0, 0, fields, &table) == 0);
    REQUIRE(AdsCloseTable(table) == 0);
    REQUIRE(AdsDisconnect(local) == 0);
    CHECK_FALSE(fs::exists(create));
    CHECK_FALSE(fs::exists(trace));
    // Recovery files are intentionally not diagnostics.
    CHECK(fs::exists(dir / "openads.txlog"));
    openads::network::Server server;
    REQUIRE(server.start("127.0.0.1", 0).has_value());
    std::string uri = "tcp://127.0.0.1:" + std::to_string(server.port()) + "/" + root;
    ADSHANDLE conn = 0;
    REQUIRE(AdsConnect60(reinterpret_cast<UNSIGNED8*>(uri.data()), ADS_REMOTE_SERVER,
                          nullptr, nullptr, 0, &conn) == 0);
    REQUIRE(AdsOpenTable(conn, name, nullptr, ADS_CDX, 0, 0, 0, 0, &table) == 0);
    REQUIRE(AdsCloseTable(table) == 0);
    CHECK_FALSE(fs::exists(trace));
    set_logging_enabled(true);
    UNSIGNED8 alias[] = "VISIBLE_ALIAS";
    REQUIRE(AdsOpenTable(conn, name, alias, ADS_CDX, 0, 0, 0, 0, &table) == 0);
    REQUIRE(AdsGotoTop(table) == 0);
    REQUIRE(AdsCloseTable(table) == 0);
    REQUIRE(fs::exists(trace));
    std::ifstream input(trace);
    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    CHECK(text.find("VISIBLE_ALIAS") != std::string::npos);
    CHECK(text.find("TBL_") != std::string::npos);
    CHECK(text.find("SECRET_REAL_TABLE") == std::string::npos);
    CHECK(text.find("PRIVATE_FIELD") == std::string::npos);
    const auto size = fs::file_size(trace);
    set_logging_enabled(false);
    REQUIRE(AdsOpenTable(conn, name, alias, ADS_CDX, 0, 0, 0, 0, &table) == 0);
    REQUIRE(AdsGotoTop(table) == 0);
    REQUIRE(AdsCloseTable(table) == 0);
    REQUIRE(AdsDisconnect(conn) == 0);
    CHECK(fs::file_size(trace) == size);
    server.stop();
#if defined(_WIN32)
    _putenv_s("OPENADS_WIRE_TRACE", "");
    _putenv_s("OPENADS_WIRE_TRACE_FILE", "");
    _putenv_s("OPENADS_CREATE_TABLE_DIAG_FILE", "");
#else
    unsetenv("OPENADS_WIRE_TRACE");
    unsetenv("OPENADS_WIRE_TRACE_FILE");
    unsetenv("OPENADS_CREATE_TABLE_DIAG_FILE");
#endif
    fs::remove_all(dir, ec);
}
