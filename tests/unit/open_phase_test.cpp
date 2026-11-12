#include "doctest.h"
#include <algorithm>
#include "util/open_phase.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
namespace {
namespace diag = openads::util::open_phase;
std::string contents(const std::filesystem::path& p) {
    std::ifstream f(p); return {std::istreambuf_iterator<char>(f), {}};
}
}
TEST_CASE("open phase opt-in bounded numeric sink and nested thread capture") {
    auto dir = std::filesystem::temp_directory_path() / "open_phase79";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    const auto log = dir / "phases.log";
    diag::configure("");
    CHECK_FALSE(diag::enabled());
    { diag::Request r(1, true); diag::Phase p(diag::Part::Engine); }
    CHECK_FALSE(std::filesystem::exists(log));
    diag::configure(log.string());
    REQUIRE(diag::enabled());
    { diag::Request r(1, false); diag::Phase p(diag::Part::Header); }
    CHECK(contents(log).empty());
    {
        diag::Request outer(7, true);
        diag::driver_open();
        { diag::Phase p(diag::Part::Header); }
        { diag::Request inner(8, true); diag::driver_open(); inner.result(99); }
        diag::driver_open(); outer.result(14);
    }
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 8; ++i) threads.emplace_back([i] {
        for (unsigned j = 0; j < 50; ++j) {
            diag::Request r(100 + i, true);
            diag::driver_open();
            diag::Phase p(diag::Part::Handle);
            r.result(14);
        }
    });
    for (auto& t : threads) t.join();
    diag::configure("");
    auto text = contents(log);
    CHECK(text.find("session=7") != std::string::npos);
    CHECK(text.find("drivers=2 reply_opcode=14") != std::string::npos);
    CHECK(text.find("session=8") != std::string::npos);
    CHECK(text.find("drivers=1 reply_opcode=99") != std::string::npos);
    CHECK(text.find(dir.string()) == std::string::npos);
    std::istringstream rows(text); std::string row; unsigned count = 0;
    while (std::getline(rows,row)) {
        ++count;
        CHECK(row.rfind("OPEN79 session=", 0) == 0);
        CHECK(row.find("reply_submit_us=") != std::string::npos);
        CHECK(row.find("fields_us=") != std::string::npos);
        CHECK(row.size() < 600);
    }
    CHECK(count == 402);
    // A sink failure only disables diagnostics; no exception escapes.
    diag::configure((dir / "no-such-dir" / "x").string());
    CHECK_FALSE(diag::enabled());
    { diag::Request r(2, true); }
    // Existing oversized file is neither truncated nor allowed to grow.
    { std::ofstream f(log, std::ios::binary | std::ios::trunc); f.seekp(8*1024*1024); f.put('x'); }
    const auto before = std::filesystem::file_size(log);
    diag::configure(log.string());
    { diag::Request r(3, true); }
    CHECK_FALSE(diag::enabled());
    diag::configure("");
    CHECK(std::filesystem::file_size(log) == before);
    std::filesystem::remove_all(dir);
}

#include "network/client.h"
#include "network/server.h"
#include <cstring>
TEST_CASE("open phase real wire opens retain metadata and failure results OFF and ON") {
    auto dir = std::filesystem::temp_directory_path() / "open_phase_wire79";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    const auto log = dir / "capture.log";
    unsigned char v[66]{};
    v[0]=3; v[8]=65; v[10]=5;
    std::memcpy(v+32,"PRIVATE_FIELD",13); v[43]='C'; v[48]=4; v[64]=13; v[65]=26;
    { std::ofstream f(dir/"PRIVATE_TABLE.dbf",std::ios::binary); f.write(reinterpret_cast<char*>(v),66); }
    openads::network::Server srv;
    REQUIRE(srv.start("127.0.0.1",0).has_value());
    openads::network::RemoteConnection c;
    REQUIRE(c.connect("127.0.0.1",srv.port(),dir.string()).has_value());
    diag::configure("");
    auto off=c.open_table("PRIVATE_TABLE.dbf");
    REQUIRE(off.has_value());
    REQUIRE(c.close_table(off.value().id).has_value());
    auto missing_off=c.open_table("PRIVATE_MISSING.dbf");
    REQUIRE_FALSE(missing_off.has_value());
    diag::configure(log.string());
    auto on=c.open_table("PRIVATE_TABLE.dbf");
    REQUIRE(on.has_value());
    CHECK(on.value().has_schema == off.value().has_schema);
    CHECK(on.value().record_length == off.value().record_length);
    REQUIRE(on.value().fields.size() == off.value().fields.size());
    CHECK(on.value().fields.at(0).name == off.value().fields.at(0).name);
    REQUIRE(c.close_table(on.value().id).has_value());
    auto missing_on=c.open_table("PRIVATE_MISSING.dbf");
    REQUIRE_FALSE(missing_on.has_value());
    CHECK(missing_on.error().code == missing_off.error().code);
    c.disconnect(); srv.stop(); diag::configure("");
    const auto text=contents(log);
    CHECK(std::count(text.begin(),text.end(),'\n') == 2);
    CHECK(text.find("drivers=1") != std::string::npos);
    CHECK(text.find("header_us=") != std::string::npos);
    CHECK(text.find("PRIVATE") == std::string::npos);
    CHECK(text.find(dir.string()) == std::string::npos);
    CHECK(text.find(".dbf") == std::string::npos);
    std::filesystem::remove_all(dir);
}
