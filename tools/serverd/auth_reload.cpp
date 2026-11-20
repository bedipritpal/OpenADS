#include "auth_reload.h"

#include "config_ini.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>

namespace openads::serverd {
namespace {

// An openads.ini is a handful of KB; anything larger is not a config we
// wrote, so bound the read rather than slurp an arbitrary file.
constexpr std::size_t kMaxIniBytes = 1u << 20;

// FNV-1a 64-bit over the file bytes: change detection only, not a
// security hash, so a fast non-crypto hash is the right tool.
std::uint64_t fnv1a64(const std::string& bytes) {
    std::uint64_t h = 14695981039346656037ull;
    for (char ch : bytes) {
        // Explicit widening: bytes above 0x7F hash as their unsigned
        // value on both signed- and unsigned-char platforms.
        h ^= static_cast<unsigned char>(ch);
        h *= 1099511628211ull;
    }
    return h;
}

// Whole file as a byte string, or empty optional on any failure.
// Throws nothing.
bool read_file(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return false;
    const auto end = in.tellg();
    if (end < 0 || static_cast<unsigned long long>(end) > kMaxIniBytes)
        return false;
    out.resize(static_cast<std::size_t>(end));
    in.seekg(0);
    if (!out.empty()) in.read(&out[0], static_cast<std::streamsize>(out.size()));
    return in.good() || in.eof();
}

struct ReloadState {
    std::string ini_path;
    std::vector<std::pair<std::string, std::string>> cli_auth_users;
    std::uint64_t last_checked_hash = 0;
    bool          have_last_hash    = false;
};

}  // namespace

std::function<bool(std::vector<std::pair<std::string, std::string>>& out)>
make_auth_user_reloader(
    std::string ini_path,
    std::vector<std::pair<std::string, std::string>> cli_auth_users) {
    auto state = std::make_shared<ReloadState>(
        ReloadState{std::move(ini_path), std::move(cli_auth_users), 0, false});
    return [state](std::vector<std::pair<std::string, std::string>>& out) -> bool {
        // Read twice and require both reads to agree: a file observed
        // while an editor is writing it is skipped this cycle.
        std::string first, second;
        if (!read_file(state->ini_path, first)) return false;
        if (!read_file(state->ini_path, second)) return false;
        if (first != second) return false;
        const std::uint64_t hash = fnv1a64(first);
        if (state->have_last_hash && hash == state->last_checked_hash)
            return false;  // unchanged since the last check, applied or not

        IniConfig cfg;
        std::string error;
        if (!parse_ini(first, cfg, error)) {
            // Keep-last-good: remember the rejected content so it is not
            // re-parsed every interval; the next edit changes the hash.
            state->last_checked_hash = hash;
            state->have_last_hash    = true;
            return false;
        }
        if (cfg.auth_users.empty()) {
            // An empty auth_user set must never disable authentication
            // on a live server: reject, keep the current list.
            state->last_checked_hash = hash;
            state->have_last_hash    = true;
            return false;
        }
        // Startup precedence: ini entries first, CLI entries appended so
        // a same-user CLI flag wins, exactly as add_credential order at
        // startup produces.
        out = cfg.auth_users;
        out.insert(out.end(), state->cli_auth_users.begin(),
                   state->cli_auth_users.end());
        state->last_checked_hash = hash;
        state->have_last_hash    = true;
        return true;
    };
}

}  // namespace openads::serverd
