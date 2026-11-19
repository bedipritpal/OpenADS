// openads_serverd — auth_user hot reload for openads.ini.
//
// The daemon reads openads.ini once at startup; before this hook, every
// auth_user edit required a full service restart, kicking every connected
// client. This reloader lets the running server pick up auth_user changes
// only. Every other key in the file stays restart-only.
//
// Agreed operator rules (decided with the owner before implementation):
//   - Only a fully parsed file is applied. Any parse error keeps the
//     current working credential list (keep-last-good).
//   - A file with ZERO auth_user lines is rejected for the same reason:
//     an empty list must never switch authentication off on a live server.
//   - Removals and password changes apply to NEW logins only; established
//     sessions are never disconnected by a reload.
//   - The reloader produces no log output (owner preference) and never
//     throws.
//
// Change detection: the file is read twice per check and compared, so a
// file observed mid-write is skipped until the next check; unchanged
// content (FNV-1a over the bytes) is not re-parsed. This bounds the work
// per check; it cannot prove an editor finished writing the file the
// operator intended, only that the two reads agreed.

#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace openads::serverd {

// Returns the reload source for Server::set_credential_reload.
//
// `ini_path` is the openads.ini the daemon started with (--config).
// `cli_auth_users` are the --auth-user pairs from the command line, in
// order; they are appended after the file's auth_user lines so the CLI
// wins a same-user collision, reproducing the startup precedence
// (defaults < ini < CLI).
//
// The returned callable contract matches Server::CredentialReloadFn:
// returns true and fills `out` with the complete replacement list when a
// changed, fully valid file with at least one auth_user line was read;
// returns false (out untouched) for no-change and for every failure
// (missing/unreadable file, mid-write observation, parse error, zero
// auth_user lines).
std::function<bool(std::vector<std::pair<std::string, std::string>>& out)>
make_auth_user_reloader(
    std::string ini_path,
    std::vector<std::pair<std::string, std::string>> cli_auth_users);

}  // namespace openads::serverd
