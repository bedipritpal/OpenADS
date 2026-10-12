#include "doctest.h"
#include "openads/ace.h"
#include "drivers/cdx/cdx_driver.h"
#include "network/client.h"
#include "network/server.h"
#include <array>
#include <fstream>
#include <filesystem>
#include <vector>
using openads::drivers::cdx::CdxDriver;
using openads::drivers::DriverOpenMode;
namespace fs=std::filesystem;
// Focused physical-count regressions. Direct wire requests intentionally avoid
// changing the separate ACE client count-cache policy.
namespace {
void count_seed(const fs::path& p, std::uint32_t header_count = 3) {
    std::array<std::uint8_t, 65> h{};
    h[0] = 3;
    h[4] = static_cast<std::uint8_t>(header_count);
    h[8] = 65;
    h[10] = 6;
    h[32] = 'V'; h[43] = 'C'; h[48] = 5; h[64] = 0x0D;
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char*>(h.data()), h.size());
    for (int n = 0; n < 3; ++n) f.write(" AAAAA", 6);
    f.put(0x1A);
}
void count_header(const fs::path& p, std::uint32_t n) {
    std::fstream f(p, std::ios::in | std::ios::out | std::ios::binary);
    std::array<std::uint8_t, 4> b{{static_cast<std::uint8_t>(n),
        static_cast<std::uint8_t>(n >> 8), static_cast<std::uint8_t>(n >> 16),
        static_cast<std::uint8_t>(n >> 24)}};
    f.seekp(4); f.write(reinterpret_cast<const char*>(b.data()), b.size());
}
fs::path count_dir(const char* name) {
    auto d = fs::temp_directory_path() / name;
    std::error_code ec; fs::remove_all(d, ec); fs::create_directories(d);
    return d;
}
}

TEST_CASE("Physical count: ordinary DBF refresh follows body size, not stale header") {
    const auto dir = count_dir("openads_physical_count_body");
    const auto p = dir / "data.dbf";
    count_seed(p);
    {
        CdxDriver driver;
        REQUIRE(driver.open(p.string(), DriverOpenMode::Shared).has_value());
        count_header(p, 99);
        REQUIRE(driver.refresh_record_count_from_disk().has_value());
        CHECK(driver.record_count() == 3);
        count_header(p, 1);
        REQUIRE(driver.refresh_record_count_from_disk().has_value());
        CHECK(driver.record_count() == 3);
        // A peer's shrink with a stale high header must not resurrect rows.
        count_header(p, 99);
        fs::resize_file(p, 65 + 6);
        REQUIRE(driver.refresh_record_count_from_disk().has_value());
        CHECK(driver.record_count() == 1);
        fs::resize_file(p, 65);
        REQUIRE(driver.refresh_record_count_from_disk().has_value());
        CHECK(driver.record_count() == 0);
        fs::resize_file(p, 65 + 1); // optional EOF byte, no records
        REQUIRE(driver.refresh_record_count_from_disk().has_value());
        CHECK(driver.record_count() == 0);
        fs::resize_file(p, 64); // enough bytes to read count, short DBF header
        auto short_body = driver.refresh_record_count_from_disk();
        REQUIRE_FALSE(short_body.has_value());
        CHECK(short_body.error().code == 5103);
        fs::resize_file(p, 4);
        auto short_header = driver.refresh_record_count_from_disk();
        REQUIRE_FALSE(short_header.has_value());
        CHECK(short_header.error().code == 5103);
    }
    fs::remove_all(dir);
}

TEST_CASE("Physical count: local ACE returns refresh errors without outputting stale count") {
    const auto dir = count_dir("openads_physical_count_ace");
    const auto p = dir / "data.dbf";
    count_seed(p);
    auto path = dir.string();
    ADSHANDLE connection = 0, table = 0;
    REQUIRE(AdsConnect60(reinterpret_cast<UNSIGNED8*>(path.data()),
        ADS_LOCAL_SERVER, nullptr, nullptr, 0, &connection) == AE_SUCCESS);
    UNSIGNED8 name[] = "data.dbf";
    REQUIRE(AdsOpenTable(connection, name, nullptr, ADS_CDX, ADS_ANSI,
        ADS_COMPATIBLE_LOCKING, ADS_IGNORERIGHTS, ADS_SHARED, &table) == AE_SUCCESS);
    count_header(p, 99);
    UNSIGNED32 count = 0;
    REQUIRE(AdsGetRecordCount(table, ADS_IGNOREFILTERS, &count) == AE_SUCCESS);
    CHECK(count == 3);
    fs::resize_file(p, 4);
    count = 0xDEADBEEFu;
    CHECK(AdsGetRecordCount(table, ADS_IGNOREFILTERS, &count) == 5103);
    CHECK(count == 0xDEADBEEFu);
    CHECK(AdsCloseTable(table) == AE_SUCCESS);
    CHECK(AdsDisconnect(connection) == AE_SUCCESS);
    fs::remove_all(dir);
}

TEST_CASE("Physical count: direct remote refresh sees peer append and forwards disk errors") {
    const auto dir = count_dir("openads_physical_count_wire");
    const auto p = dir / "data.dbf";
    count_seed(p);
    openads::network::Server server;
    REQUIRE(server.start("127.0.0.1", 0).has_value());
    openads::network::RemoteConnection client;
    REQUIRE(client.connect("127.0.0.1", server.port(), dir.string()).has_value());
    auto opened = client.open_table("data.dbf");
    REQUIRE(opened.has_value());
    const auto id = opened.value().id;
    REQUIRE(client.goto_top(id).has_value());
    auto count = client.record_count(id);
    REQUIRE(count.has_value()); CHECK(count.value() == 3);
    {
        CdxDriver peer;
        REQUIRE(peer.open(p.string(), DriverOpenMode::Shared).has_value());
        std::vector<std::uint8_t> row(peer.record_length(), ' ');
        REQUIRE(peer.append_record_raw(row.data(), row.size()).has_value());
    }
    // Make the header lag behind the new physical row, then overstate it.
    count_header(p, 3);
    count = client.record_count(id);
    REQUIRE(count.has_value()); CHECK(count.value() == 4);
    count_header(p, 99);
    count = client.record_count(id);
    REQUIRE(count.has_value()); CHECK(count.value() == 4);
    fs::resize_file(p, 65); // peer emptied the body without updating header
    count = client.record_count(id);
    REQUIRE(count.has_value()); CHECK(count.value() == 0);
    fs::resize_file(p, 4);
    count = client.record_count(id);
    REQUIRE_FALSE(count.has_value()); CHECK(count.error().code == 5103);
    CHECK(client.close_table(id).has_value());
    client.disconnect(); server.stop();
    fs::remove_all(dir);
}
