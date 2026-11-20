#include "doctest.h"
#include "tools/serverd/auth_reload.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

using PairList = std::vector<std::pair<std::string, std::string>>;

fs::path temp_ini(const std::string& tag) {
    return fs::temp_directory_path() / (std::string("openads_auth_reload_") + tag);
}

void write_ini(const fs::path& p, const std::string& text) {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out << text;
}

struct IniGuard {
    fs::path path;
    ~IniGuard() { std::error_code ec; fs::remove(path, ec); }
};

}  // namespace

TEST_CASE("auth_user reloader applies a changed valid file") {
    IniGuard g{temp_ini("apply.ini")};
    write_ini(g.path, "auth_user = alice:pw1\n");
    auto reload = openads::serverd::make_auth_user_reloader(g.path.string(), {});

    PairList out;
    REQUIRE(reload(out));
    REQUIRE(out.size() == 1);
    CHECK(out[0].first == "alice");
    CHECK(out[0].second == "pw1");

    // Unchanged content: no re-parse, no new list.
    PairList out2;
    CHECK_FALSE(reload(out2));
    CHECK(out2.empty());

    // An added client is picked up.
    write_ini(g.path, "auth_user = alice:pw1\nauth_user = bob:pw2\n");
    PairList out3;
    REQUIRE(reload(out3));
    REQUIRE(out3.size() == 2);
    CHECK(out3[1].first == "bob");
}

TEST_CASE("auth_user reloader detects a change in high-bit bytes") {
    IniGuard g{temp_ini("highbit.ini")};
    // Bytes above 0x7F (e.g. UTF-8 in a comment) must feed the change
    // hash as unsigned values; a flip between two high-bit bytes is a
    // real change even where plain char is signed.
    write_ini(g.path, "# caf\xC3\xA9\nauth_user = alice:pw1\n");
    auto reload = openads::serverd::make_auth_user_reloader(g.path.string(), {});

    PairList out;
    REQUIRE(reload(out));
    REQUIRE(out.size() == 1);

    PairList out2;
    CHECK_FALSE(reload(out2));  // unchanged

    write_ini(g.path, "# caf\xC3\xA8\nauth_user = alice:pw1\n");
    PairList out3;
    REQUIRE(reload(out3));  // one high-bit byte differs: detected
    REQUIRE(out3.size() == 1);
    CHECK(out3[0].first == "alice");
}

TEST_CASE("auth_user reloader keeps last good on parse error, then recovers") {
    IniGuard g{temp_ini("bad.ini")};
    write_ini(g.path, "auth_user = alice:pw1\n");
    auto reload = openads::serverd::make_auth_user_reloader(g.path.string(), {});
    PairList out;
    REQUIRE(reload(out));

    // Broken edit (unknown key): rejected, current list kept.
    write_ini(g.path, "auth_user = alice:pw1\nbogus_key = 1\n");
    CHECK_FALSE(reload(out));

    // Fixing the file applies again.
    write_ini(g.path, "auth_user = alice:pw1\nauth_user = carol:pw3\n");
    REQUIRE(reload(out));
    REQUIRE(out.size() == 2);
    CHECK(out[1].first == "carol");
}

TEST_CASE("auth_user reloader rejects zero auth_user lines") {
    IniGuard g{temp_ini("zero.ini")};
    write_ini(g.path, "auth_user = alice:pw1\n");
    auto reload = openads::serverd::make_auth_user_reloader(g.path.string(), {});
    PairList out;
    REQUIRE(reload(out));

    // A valid file with no auth_user lines must never disable auth on a
    // live server: rejected even though it parses.
    write_ini(g.path, "port = 6262\n");
    CHECK_FALSE(reload(out));
}

TEST_CASE("auth_user reloader rejects malformed auth_user and missing file") {
    IniGuard g{temp_ini("malformed.ini")};
    write_ini(g.path, "auth_user = justaname\n");
    auto reload = openads::serverd::make_auth_user_reloader(g.path.string(), {});
    PairList out;
    CHECK_FALSE(reload(out));

    auto missing = openads::serverd::make_auth_user_reloader(
        temp_ini("does_not_exist.ini").string(), {});
    CHECK_FALSE(missing(out));
}

TEST_CASE("auth_user reloader appends CLI pairs so CLI wins same-user") {
    IniGuard g{temp_ini("cli.ini")};
    write_ini(g.path, "auth_user = alice:ini_pw\nauth_user = bob:pw2\n");
    PairList cli{{"alice", "cli_pw"}};
    auto reload = openads::serverd::make_auth_user_reloader(g.path.string(), cli);

    PairList out;
    REQUIRE(reload(out));
    // ini first, CLI last: when Server applies the list in order, the
    // CLI entry overwrites the ini one, matching startup precedence.
    REQUIRE(out.size() == 3);
    CHECK(out[0] == std::make_pair(std::string("alice"), std::string("ini_pw")));
    CHECK(out[1] == std::make_pair(std::string("bob"), std::string("pw2")));
    CHECK(out[2] == std::make_pair(std::string("alice"), std::string("cli_pw")));
}
