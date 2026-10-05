#include "doctest.h"
#include "mgmt/mg_health.h"
#include "network/server.h"
#include "network/mg_wire.h"
#include "network/socket.h"
#include "openads/ace.h"
#include <string>
#include <vector>

TEST_CASE("health JSON is aggregate-only with wide counters and honest unknowns") {
    openads::mgmt::MgSnapshot snap;
    snap.server_type = 1;
    snap.workareas = snap.tables = 3;
    snap.max_workareas = 8;
    snap.bytes_in = 4294967296ULL;
    snap.table_list = {{"/private/a.dbf", "private-user", 1},
                       {"/private/a.dbf", "private-user", 1},
                       {"/private/b.dbf", "private-user", 1}};
    auto json = openads::mgmt::health_json(snap, "version\"\n");
    CHECK(json.find("\"version\":\"version\\\"\\u000a\"") != std::string::npos);
    CHECK(json.find("\"workareas\":{\"current\":3,\"max_used\":8") != std::string::npos);
    CHECK(json.find("\"distinct_table_paths\":2") != std::string::npos);
    CHECK(json.find("\"bytes_in\":4294967296") != std::string::npos);
    CHECK(json.find("\"parked_handles\":null") != std::string::npos);
    CHECK(json.find("\"rejected\":null") != std::string::npos);
    CHECK(json.find("/private") == std::string::npos);
    CHECK(json.find("private-user") == std::string::npos);
}

TEST_CASE("health C ABI validates management handles and never truncates JSON") {
    UNSIGNED8 local[] = "local";
    ADSHANDLE handle = 0;
    REQUIRE(AdsMgConnect(local, nullptr, nullptr, &handle) == 0);
    UNSIGNED32 len = 0;
    CHECK(OAdsGetServerStats(0, nullptr, &len) != 0);
    CHECK(OAdsGetServerStats(handle, nullptr, nullptr) != 0);
    REQUIRE(OAdsGetServerStats(handle, nullptr, &len) == AE_INSUFFICIENT_BUFFER);
    REQUIRE(len > 10);
    char small[] = "XXXX";
    UNSIGNED32 capacity = sizeof(small);
    CHECK(OAdsGetServerStats(handle, reinterpret_cast<UNSIGNED8*>(small), &capacity) == AE_INSUFFICIENT_BUFFER);
    CHECK(std::string(small) == "XXXX");
    std::vector<UNSIGNED8> buffer(8192);
    len = static_cast<UNSIGNED32>(buffer.size());
    REQUIRE(OAdsGetServerStats(handle, buffer.data(), &len) == 0);
    CHECK(buffer[len - 1] == 0);
    CHECK(std::string(reinterpret_cast<char*>(buffer.data())).find("\"scope\":\"local_process\"") != std::string::npos);
    REQUIRE(AdsMgDisconnect(handle) == 0);
    CHECK(OAdsGetServerStats(handle, buffer.data(), &len) != 0);
}

TEST_CASE("health remote query uses management authentication and server-side measurement") {
    openads::network::Server server;
    server.set_max_sessions(123, "openads.ini:max_sessions");
    server.set_daemon_hardening(true);
    server.add_credential("admin", "test-only-password");
    REQUIRE(server.start("127.0.0.1", 0));
    openads::network::Server::SessionInfo info;
    info.user = "unexposed-user";
    info.open_tables = 3;
    info.open_table_names = {"/private/a.dbf", "/private/a.dbf", "/private/b.dbf"};
    const auto id = server.register_session(info);
    std::string endpoint = "127.0.0.1:" + std::to_string(server.port());
    SUBCASE("plain management endpoint") {}
    SUBCASE("tcp scheme and trailing slash") { endpoint = "tcp://" + endpoint + "/"; }
    SUBCASE("uppercase TCP scheme and trailing slash") { endpoint = "TCP://" + endpoint + "/"; }
    UNSIGNED8 user[] = "admin", password[] = "test-only-password";
    ADSHANDLE handle = 0;
    REQUIRE(AdsMgConnect(reinterpret_cast<UNSIGNED8*>(endpoint.data()), user, password, &handle) == 0);
    UNSIGNED8 buffer[8192];
    UNSIGNED32 len = sizeof(buffer);
    REQUIRE(OAdsGetServerStats(handle, buffer, &len) == 0);
    std::string json(reinterpret_cast<char*>(buffer));
    CHECK(json.find("\"scope\":\"server\"") != std::string::npos);
    CHECK(json.find("\"workareas\":{\"current\":3") != std::string::npos);
    CHECK(json.find("\"distinct_table_paths\":2") != std::string::npos);
    CHECK(json.find("\"server_port\":" + std::to_string(server.port())) != std::string::npos);
    CHECK(json.find("\"max_sessions\":123") != std::string::npos);
    CHECK(json.find("\"max_sessions_source\":\"openads.ini:max_sessions\"") != std::string::npos);
    CHECK(json.find("unexposed-user") == std::string::npos);
    CHECK(json.find("/private") == std::string::npos);
    CHECK(json.find("test-only-password") == std::string::npos);
    CHECK(AdsMgDisconnect(handle) == 0);
    // The additive read is allowed for the EXISTING credential-free loopback
    // management session, but never for a socket without MgConnect.
    server.stop();
    server.set_daemon_hardening(true);
    openads::network::Server loopback;
    loopback.set_daemon_hardening(true);
    REQUIRE(loopback.start("127.0.0.1", 0));
    endpoint = "127.0.0.1:" + std::to_string(loopback.port());
    REQUIRE(AdsMgConnect(reinterpret_cast<UNSIGNED8*>(endpoint.data()), nullptr, nullptr, &handle) == 0);
    len = sizeof(buffer);
    CHECK(OAdsGetServerStats(handle, buffer, &len) == 0);
    CHECK(AdsMgResetCommStats(handle) != 0);
    CHECK(AdsMgDisconnect(handle) == 0);
    auto connected = openads::network::connect_tcp("127.0.0.1", loopback.port());
    REQUIRE(connected);
    auto socket = connected.value();
    openads::network::Frame frame;
    frame.opcode = openads::network::Opcode::MgRequest;
    const auto body = openads::network::encode_mg_request(openads::network::MgRequestKind::HealthJson, 0);
    frame.payload.assign(body.begin(), body.end());
    REQUIRE(openads::network::write_frame(socket, frame));
    auto reply = openads::network::read_frame(socket);
    REQUIRE(reply);
    CHECK(reply.value().opcode != openads::network::Opcode::MgReplyAck);
    openads::network::sock_close(socket);
    (void)id;
    loopback.stop();
}

TEST_CASE("health workarea peak retains transient opens without a snapshot") {
    openads::network::Server server;
    openads::network::Server::SessionInfo a;
    a.open_tables = 2;
    const auto first = server.register_session(a);
    const auto second = server.register_session({});
    server.add_session_table(second, 1000, "same.dbf");
    // Close before the first sample, as in a short storm.
    server.add_session_table(second, -1000, "same.dbf");
    auto snapshot = server.build_mg_snapshot();
    CHECK(snapshot.workareas == 2);
    CHECK(snapshot.max_workareas >= 1002);
    CHECK(snapshot.max_tables >= 1002);
    const auto peak = snapshot.max_workareas;
    server.add_session_table(first, -100, "same.dbf"); // clamped close
    server.unregister_session(second);
    server.unregister_session(first);
    server.unregister_session(first); // duplicate cleanup cannot underflow
    snapshot = server.build_mg_snapshot();
    CHECK(snapshot.workareas == 0);
    CHECK(snapshot.max_workareas == peak);
    const auto third = server.register_session({});
    server.add_session_table(third, 3, "same.dbf");
    snapshot = server.build_mg_snapshot();
    CHECK(snapshot.workareas == 3);
    CHECK(snapshot.max_workareas == peak);
    server.unregister_session(third);
}

TEST_CASE("health explicit zero session cap remains unlimited") {
    openads::network::Server server;
    server.set_max_sessions(0, "command line:--max_sessions");
    REQUIRE(server.start("127.0.0.1", 0));
    auto snapshot = server.build_mg_snapshot();
    CHECK(snapshot.max_sessions == 0);
    CHECK(snapshot.max_sessions_source == "command line:--max_sessions");
    auto json = openads::mgmt::health_json(snapshot, "test");
    CHECK(json.find("\"max_sessions\":0") != std::string::npos);
    server.stop();
    openads::mgmt::MgSnapshot local;
    json = openads::mgmt::health_json(local, "test");
    CHECK(json.find("\"max_sessions\":null") != std::string::npos);
}
