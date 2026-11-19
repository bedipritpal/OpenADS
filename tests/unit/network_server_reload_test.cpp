#include "doctest.h"
#include "network/server.h"

#include <atomic>
#include <chrono>
#include <memory>
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
