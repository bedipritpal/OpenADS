// auth:// hot-reload proof (public V1 mock module, no production material):
// after a module-approved auth, the server checks the returned pair against
// its credential map - this proves the reloaded map governs that gate.
//
//   1. auth:// login succeeds with the mock's usr:pw pair in the map.
//   2. A reload removing usr makes the next module-approved login FAIL.
//   3. A reload restoring usr makes it succeed again.
//   4. A blob the mock rejects is never authenticated.
//
// Ordering respects the server's per-IP login backoff (a security
// control, not to be probed through): every intentional rejection trips
// it, so positive checks run before the next intentional failure and a
// bounded wait clears the one unavoidable rejection (step 2) before the
// restore check.
//
// Usage: auth_reload_probe <openads_auth_worker> <auth-reload-test-only.hrb>
#include "network/server.h"
#include "network/client.h"
#include "network/tls_transport.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace openads::network;

int main(int argc, char** argv) {
    assert(argc == 3);
    auto dir = std::filesystem::current_path() / "auth-reload-v1-proof";
    std::filesystem::create_directories(dir);
    auto worker = std::make_shared<AuthWorker>(argv[1], argv[2]);
    assert(worker->probe() && !worker->mutual_required());
    auto cfg = make_auth_tls_config();
    assert(cfg);

    Server srv;
    srv.set_data_dir(dir.string());
    srv.add_credential("usr", "pw");
    srv.set_auth_worker(worker);
    // Reload source under test. v1 removes usr (adds an unrelated pair);
    // v2 restores the startup list.
    std::atomic<int> version{0};
    srv.set_credential_reload(
        [&](std::vector<std::pair<std::string, std::string>>& out) -> bool {
            const int v = version.load();
            if (v == 1)      out = {{"other", "pw2"}};
            else if (v == 2) out = {{"usr", "pw"}};
            else             return false;
            return true;
        },
        /*interval_seconds=*/0);
    assert(srv.set_tls(cfg.value()));
    assert(srv.start("127.0.0.1", 0));

    auto tls = cfg.value();
    tls.insecure_skip_verify = true;
    tls.auth_tls13_only      = true;
    const std::string path = dir.string();
    const auto login = [&](const char* suffix) {
        auto transport = connect_tls("127.0.0.1", srv.port(), tls);
        if (!transport) return false;
        RemoteConnection client;
        auto challenge = client.begin_auth(std::move(transport).value(), path);
        if (!challenge) return false;
        std::string blob(
            reinterpret_cast<const char*>(challenge.value().data()),
            challenge.value().size());
        blob += suffix;
        return static_cast<bool>(client.complete_auth(blob));
    };

    assert(login("test-only"));    // module approves; usr:pw accepted
    version = 1;
    assert(!login("test-only"));   // module still approves; usr removed
    // That rejection is one per-IP login failure; the server enforces a
    // 1s backoff (1 << (failures-1)). Wait it out rather than trip a
    // security control the probe is not testing.
    std::this_thread::sleep_for(std::chrono::seconds(2));
    version = 2;
    assert(login("test-only"));    // usr restored
    assert(!login("wrong-suffix"));  // module rejects: never authenticated
    srv.stop();
    std::cerr << "auth_reload_v1 ok\n";
    return 0;
}
