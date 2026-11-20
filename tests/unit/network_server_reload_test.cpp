#include "doctest.h"
#include "network/server.h"
#include "network/socket.h"
#include "network/wire.h"
#include "openads/ace.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using openads::network::Server;

namespace {

using PairList = std::vector<std::pair<std::string, std::string>>;

}  // namespace

TEST_CASE("credential reload swaps the whole map on success") {
    Server srv;
    srv.add_credential("old", "oldpw");
    REQUIRE(srv.validate_credential("old", "oldpw"));

    bool apply = false;
    srv.set_credential_reload(
        [&](PairList& out) -> bool {
            if (!apply) return false;
            out = {{"fresh", "newpw"}};
            return true;
        },
        /*interval_seconds=*/0);

    // No change reported: map untouched.
    srv.maybe_reload_credentials();
    CHECK(srv.validate_credential("old", "oldpw"));

    // Applied: the previous map is fully replaced (removal + addition).
    apply = true;
    srv.maybe_reload_credentials();
    CHECK(srv.validate_credential("fresh", "newpw"));
    CHECK_FALSE(srv.validate_credential("old", "oldpw"));
    CHECK(srv.require_auth());
}

TEST_CASE("credential reload keep-last-good: failure and empty list keep the map") {
    Server srv;
    srv.add_credential("keeper", "pw");

    srv.set_credential_reload(
        [](PairList&) -> bool { return false; }, 0);
    srv.maybe_reload_credentials();
    CHECK(srv.validate_credential("keeper", "pw"));

    // A source that reports success with an empty list is also refused:
    // authentication must never switch off through a reload.
    srv.set_credential_reload(
        [](PairList& out) -> bool {
            out.clear();
            return true;
        },
        0);
    srv.maybe_reload_credentials();
    CHECK(srv.validate_credential("keeper", "pw"));
}

TEST_CASE("credential reload: additions, removals and password changes hit the next login") {
    Server srv;
    srv.add_credential("alice", "pw1");
    srv.add_credential("bob", "pw2");

    // Simulates a re-read openads.ini: alice's password changed, bob
    // removed, carol added.
    srv.set_credential_reload(
        [](PairList& out) -> bool {
            out = {{"alice", "pw1b"}, {"carol", "pw3"}};
            return true;
        },
        /*interval_seconds=*/0);
    srv.maybe_reload_credentials();

    CHECK(srv.validate_credential("alice", "pw1b"));   // new password works
    CHECK_FALSE(srv.validate_credential("alice", "pw1"));  // old one dies
    CHECK_FALSE(srv.validate_credential("bob", "pw2"));    // removal works
    CHECK(srv.validate_credential("carol", "pw3"));    // addition works
    CHECK(srv.require_auth());
}

TEST_CASE("credential reload is interval-gated") {
    Server srv;
    srv.add_credential("a", "pw");
    std::atomic<int> calls{0};
    srv.set_credential_reload(
        [&](PairList& out) -> bool {
            ++calls;
            out = {{"b", "pw"}};
            return true;
        },
        /*interval_seconds=*/3600);

    srv.maybe_reload_credentials();  // first call always checks
    srv.maybe_reload_credentials();  // inside the interval: skipped
    srv.maybe_reload_credentials();
    CHECK(calls.load() == 1);
    CHECK(srv.validate_credential("b", "pw"));
}

// KNOWN DEFECT (auth_user reload stage): password verification holds
// creds_mu_ for the whole PBKDF2 run (pre-existing session.cpp behavior),
// so readers that re-acquire it immediately can starve a reload swap for
// as long as the flood lasts - a credential removal is then delayed, not
// lost. This stress test reproduces the flood with spin-pacing and a hard
// deadline: it FAILS until a bounded-progress design lands. Do not weaken
// it; the paced variant below is the additional evidence for the swap
// semantics themselves.
TEST_CASE("credential reload STRESS: swap must land under a continuous login flood") {
    // Heap-held so the failure path can leak it: when the swap starves,
    // the driver thread stays blocked inside maybe_reload_credentials
    // and must not see a destroyed Server during teardown.
    auto srv_owner = std::make_unique<Server>();
    Server& srv = *srv_owner;
    srv.add_credential("old", "oldpw");

    std::atomic<bool> apply{false};
    std::atomic<bool> swapped{false};
    std::atomic<bool> stop{false};
    srv.set_credential_reload(
        [&](PairList& out) -> bool {
            if (!apply.load() || swapped.load()) return false;
            out = {{"fresh", "newpw"}};
            return true;
        },
        /*interval_seconds=*/0);

    // Spinning login flood: no pacing, worst case for the writer.
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&] {
            while (!stop.load()) {
                (void)srv.validate_credential("old", "oldpw");
                (void)srv.validate_credential("fresh", "newpw");
            }
        });
    }
    // The driver attempts the swap; it can itself starve inside
    // maybe_reload_credentials, so the main thread observes progress
    // from the reader side against a hard deadline: a livelock becomes
    // a deterministic failure instead of a hang.
    std::thread driver([&] {
        while (!stop.load()) srv.maybe_reload_credentials();
    });
    apply = true;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (std::chrono::steady_clock::now() < deadline) {
        if (srv.validate_credential("fresh", "newpw")) { swapped = true; break; }
    }
    stop = true;
    for (auto& t : readers) t.join();

    if (swapped) {
        driver.join();
        CHECK_FALSE(srv.validate_credential("old", "oldpw"));
    } else {
        // Defect reproduced: abandon the stuck driver and its Server.
        driver.detach();
        srv_owner.release();
    }
    CHECK(swapped.load());  // FAILS today: swap starved by verify-under-lock flood
}

TEST_CASE("credential reload under concurrent logins keeps the map consistent") {
    Server srv;
    srv.add_credential("old", "oldpw");

    std::atomic<bool> apply{false};
    std::atomic<int>  applied{0};
    srv.set_credential_reload(
        [&](PairList& out) -> bool {
            if (!apply.load()) return false;
            // PBKDF2 hashing is deliberately slow; cap the swaps.
            if (applied.load() >= 8) return false;
            out = {{"fresh", "newpw"}};
            ++applied;
            return true;
        },
        /*interval_seconds=*/0);

    std::atomic<bool> stop{false};
    std::atomic<int>  bad{0};
    // Login-like readers. Each call sees one complete map; a swap may
    // land between the two probes, so the invariant is the safe
    // direction only: never a state where NEITHER map's login works,
    // and authentication never switches off. Readers pace themselves:
    // password verification holds the credential mutex for the whole
    // PBKDF2 run (pre-existing login-path behavior), so spin-reacquiring
    // readers would starve the swap the way a continuous login flood
    // could delay it on a live server.
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&] {
            while (!stop.load()) {
                const bool old_ok   = srv.validate_credential("old", "oldpw");
                const bool fresh_ok = srv.validate_credential("fresh", "newpw");
                if (!old_ok && !fresh_ok) ++bad;
                if (!srv.require_auth()) ++bad;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });
    }
    // Concurrent reloaders, including the transition.
    std::vector<std::thread> reloaders;
    for (int i = 0; i < 4; ++i) {
        reloaders.emplace_back([&] {
            for (int n = 0; n < 10; ++n) srv.maybe_reload_credentials();
        });
    }
    apply = true;
    for (auto& t : reloaders) t.join();
    srv.maybe_reload_credentials();
    stop = true;
    for (auto& t : readers) t.join();

    CHECK(bad.load() == 0);
    CHECK(applied.load() >= 1);
    CHECK(srv.validate_credential("fresh", "newpw"));
    CHECK_FALSE(srv.validate_credential("old", "oldpw"));
}

TEST_CASE("credential reload: a throwing source keeps last good and the gate recovers") {
    Server srv;
    srv.add_credential("keeper", "pw");
    int calls = 0;
    srv.set_credential_reload(
        [&](PairList& out) -> bool {
            if (++calls == 1) throw std::runtime_error("broken source");
            out = {{"keeper2", "pw2"}};
            return true;
        },
        /*interval_seconds=*/0);

    srv.maybe_reload_credentials();  // throws inside: contained
    CHECK(srv.validate_credential("keeper", "pw"));

    // The gate and the reload path recover: a later valid source applies.
    srv.maybe_reload_credentials();
    CHECK(srv.validate_credential("keeper2", "pw2"));
    CHECK_FALSE(srv.validate_credential("keeper", "pw"));
}

TEST_CASE("credential reload never accepts failed credentials across swaps") {
    Server srv;
    srv.add_credential("u", "right");
    std::atomic<bool> apply{false};
    srv.set_credential_reload(
        [&](PairList& out) -> bool {
            if (!apply.load()) return false;
            out = {{"u", "right2"}};
            return true;
        },
        /*interval_seconds=*/0);
    apply = true;
    for (int i = 0; i < 5; ++i) {
        srv.maybe_reload_credentials();
        CHECK_FALSE(srv.validate_credential("u", "wrong"));
        CHECK_FALSE(srv.validate_credential("ghost", "right"));
        CHECK(srv.require_auth());
    }
    CHECK(srv.validate_credential("u", "right2"));
}

namespace {

// Real Connect frame against a running server, over a real TCP socket.
openads::network::Opcode try_tcp_login(openads::network::Server& srv,
                                       const std::string& dir,
                                       const char* user,
                                       const char* pw) {
    using namespace openads::network;
    auto cli = connect_tcp("127.0.0.1", srv.port());
    REQUIRE(cli.has_value());
    Socket cs = cli.value();
    Frame req;
    req.opcode = Opcode::Connect;
    auto pushlen = [](std::vector<std::uint8_t>& o, std::uint16_t n) {
        o.push_back(static_cast<std::uint8_t>(n & 0xFFu));
        o.push_back(static_cast<std::uint8_t>((n >> 8) & 0xFFu));
    };
    auto pushstr = [&](const char* s) {
        const auto n = static_cast<std::uint16_t>(std::strlen(s));
        pushlen(req.payload, n);
        req.payload.insert(req.payload.end(), s, s + n);
    };
    pushstr(dir.c_str());
    pushstr(user);
    pushstr(pw);
    REQUIRE(write_frame(cs, req).has_value());
    auto reply = read_frame(cs);
    REQUIRE(reply.has_value());
    const Opcode op = reply.value().opcode;
    sock_close(cs);
    return op;
}

}  // namespace

TEST_CASE("credential reload E2E: plain TCP Connect logins observe the reloaded map") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "openads_reload_e2e";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);

    Server srv;
    srv.add_credential("alice", "pw1");
    std::atomic<int> version{1};
    srv.set_credential_reload(
        [&](PairList& out) -> bool {
            if (version.load() == 1) return false;
            out = {{"bob", "pw2"}};  // alice removed, bob added
            return true;
        },
        /*interval_seconds=*/0);
    REQUIRE(srv.start("127.0.0.1", 0).has_value());

    using openads::network::Opcode;
    CHECK(try_tcp_login(srv, dir.string(), "alice", "pw1") == Opcode::ConnectAck);

    // Simulates the operator saving a changed openads.ini. Positive
    // checks come first: the server's pre-existing per-IP login
    // throttling penalizes every intentional failure below.
    version = 2;
    CHECK(try_tcp_login(srv, dir.string(), "bob", "pw2") == Opcode::ConnectAck);  // addition
    CHECK(try_tcp_login(srv, dir.string(), "alice", "pw1") == Opcode::Error);      // removal
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));  // throttle window
    CHECK(try_tcp_login(srv, dir.string(), "bob", "bad") == Opcode::Error);        // wrong pw

    srv.stop();
    fs::remove_all(dir, ec);
}

TEST_CASE("credential reload E2E: management login observes the reloaded map") {
    Server srv;
    srv.add_credential("admin", "pw1");
    std::atomic<int> version{1};
    srv.set_credential_reload(
        [&](PairList& out) -> bool {
            if (version.load() == 1) return false;
            out = {{"admin2", "pw2"}};  // admin removed, admin2 added
            return true;
        },
        /*interval_seconds=*/0);
    REQUIRE(srv.start("127.0.0.1", 0).has_value());
    std::string endpoint = "127.0.0.1:" + std::to_string(srv.port());

    UNSIGNED8 admin[] = "admin", pw1[] = "pw1";
    UNSIGNED8 admin2[] = "admin2", pw2[] = "pw2";
    ADSHANDLE h = 0;
    REQUIRE(AdsMgConnect(reinterpret_cast<UNSIGNED8*>(endpoint.data()), admin, pw1, &h) == 0);
    REQUIRE(AdsMgDisconnect(h) == 0);

    // Simulates the operator saving a changed openads.ini. Positive
    // check first: pre-existing per-IP login throttling penalizes the
    // intentional failure below.
    version = 2;
    h = 0;
    REQUIRE(AdsMgConnect(reinterpret_cast<UNSIGNED8*>(endpoint.data()), admin2, pw2, &h) == 0);
    REQUIRE(AdsMgDisconnect(h) == 0);
    h = 0;
    CHECK(AdsMgConnect(reinterpret_cast<UNSIGNED8*>(endpoint.data()), admin, pw1, &h) != 0);  // removal

    srv.stop();
}
