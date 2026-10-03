
#include "doctest.h"
#include "platform/file.h"
#include "platform/fs_sandbox.h"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using openads::platform::File;
using openads::platform::OpenMode;

namespace {

struct JailGuard {
    explicit JailGuard(const std::vector<std::string>& roots) {
        auto r = File::set_data_jail(roots);
        REQUIRE(r.has_value());
    }
    ~JailGuard() { File::clear_data_jail(); }
};

fs::path canon(const fs::path& p) { return fs::weakly_canonical(p); }

fs::path fresh_dir(const char* name) {
    const fs::path d = fs::temp_directory_path() / name;
    fs::remove_all(d);
    fs::create_directories(d);
    return d;
}

void put_file(const fs::path& p, const std::string& bytes) {
    auto f = File::open(p.string(), OpenMode::CreateRW);
    REQUIRE(f.has_value());
    File file = std::move(f).value();
    if (!bytes.empty()) {
        auto w = file.write_at(0, bytes.data(), bytes.size());
        REQUIRE(w.has_value());
    }
}

} // namespace

TEST_CASE("data jail: normal opens inside the root work, companions covered") {
    const fs::path root  = fresh_dir("openads_jail_normal");
    const fs::path croot = canon(root);
    {
        JailGuard jail({croot.string()});

        for (const char* ext : {".dbf", ".cdx", ".z01", ".fpt"}) {
            const fs::path p = croot / ("t" + std::string(ext));
            put_file(p, "x");
            auto r = File::open(p.string(), OpenMode::ReadOnly);
            CHECK(r.has_value());
        }

        fs::create_directories(croot / "sub");
        const fs::path sub_file = croot / "sub" / "inner.dbf";
        put_file(sub_file, "y");
        auto r = File::open(sub_file.string(), OpenMode::ReadOnly);
        CHECK(r.has_value());

        auto dots = File::open((croot / "sub" / ".." / "t.dbf").string(),
                               OpenMode::ReadOnly);
        CHECK(dots.has_value());
    }
    fs::remove_all(root);
}

TEST_CASE("data jail: ../ escape and outside absolute paths are refused") {
    const fs::path base = fresh_dir("openads_jail_escape");
    const fs::path root = base / "data";
    fs::create_directories(root);
    const fs::path croot = canon(root);
    {
        JailGuard jail({croot.string()});

        auto esc = File::open((croot / ".." / "escape.dbf").string(),
                              OpenMode::CreateRW);
        CHECK(!esc.has_value());
        CHECK(!fs::exists(base / "escape.dbf"));

        auto abs_out = File::open((canon(base) / "abs.dbf").string(),
                                  OpenMode::CreateRW);
        CHECK(!abs_out.has_value());
        CHECK(!fs::exists(base / "abs.dbf"));
    }
    fs::remove_all(base);
}

#ifndef _WIN32

TEST_CASE("data jail: symlinked table file is refused (never-follow)") {
    const fs::path base    = fresh_dir("openads_jail_symlink");
    const fs::path root    = base / "data";
    const fs::path outside = base / "outside";
    fs::create_directories(root);
    fs::create_directories(outside);
    const fs::path croot = canon(root);

    put_file(outside / "real.dbf", "secret");
    fs::create_symlink(outside / "real.dbf", croot / "evil.dbf");
    {
        JailGuard jail({croot.string()});
        auto r = File::open((croot / "evil.dbf").string(), OpenMode::ReadOnly);
        CHECK(!r.has_value());
        auto rw = File::open((croot / "evil.dbf").string(), OpenMode::ReadWrite);
        CHECK(!rw.has_value());
    }
    fs::remove_all(base);
}

TEST_CASE("data jail: symlinked intermediate directory is refused") {
    const fs::path base    = fresh_dir("openads_jail_linkdir");
    const fs::path root    = base / "data";
    const fs::path outside = base / "outside";
    fs::create_directories(root);
    fs::create_directories(outside);
    const fs::path croot = canon(root);

    put_file(outside / "t.dbf", "secret");
    fs::create_directory_symlink(outside, croot / "linkdir");
    {
        JailGuard jail({croot.string()});
        auto r = File::open((croot / "linkdir" / "t.dbf").string(),
                            OpenMode::ReadOnly);
        CHECK(!r.has_value());
    }
    fs::remove_all(base);
}

TEST_CASE("data jail: create over a symlink is refused, target untouched") {
    const fs::path base    = fresh_dir("openads_jail_create");
    const fs::path root    = base / "data";
    const fs::path outside = base / "outside";
    fs::create_directories(root);
    fs::create_directories(outside);
    const fs::path croot = canon(root);

    put_file(outside / "target.dbf", "KEEP");
    fs::create_symlink(outside / "target.dbf", croot / "new.dbf");
    {
        JailGuard jail({croot.string()});
        auto c = File::open((croot / "new.dbf").string(), OpenMode::CreateRW);
        CHECK(!c.has_value());
        auto x = File::open((croot / "new.dbf").string(),
                            OpenMode::CreateExclusive);
        CHECK(!x.has_value());
    }
    {
        auto r = File::open((outside / "target.dbf").string(),
                            OpenMode::ReadOnly);
        REQUIRE(r.has_value());
        File f = std::move(r).value();
        char buf[4] = {};
        auto rd = f.read_at(0, buf, 4);
        REQUIRE(rd.has_value());
        CHECK(std::string(buf, rd.value()) == "KEEP");
    }
    fs::remove_all(base);
}

TEST_CASE("data jail: legacy .cdx -> .z01 symlink layout is refused") {
    const fs::path root  = fresh_dir("openads_jail_z01");
    const fs::path croot = canon(root);
    put_file(croot / "bag.z01", "z");
    fs::create_symlink("bag.z01", croot / "bag.cdx");  // relative link
    {
        JailGuard jail({croot.string()});
        auto real = File::open((croot / "bag.z01").string(), OpenMode::ReadOnly);
        CHECK(real.has_value());
        auto link = File::open((croot / "bag.cdx").string(), OpenMode::ReadOnly);
        CHECK(!link.has_value());
    }
    fs::remove_all(root);
}

TEST_CASE("data jail: in-root links and link/.. are never erased") {
    const fs::path root = fresh_dir("openads_jail_inroot_link");
    const fs::path croot = canon(root);
    fs::create_directories(croot / "real");
    put_file(croot / "real" / "t.dbf", "x");
    put_file(croot / "t.dbf", "y");
    for (const char* ext : {".dbf", ".cdx", ".fpt", ".z01"}) {
        fs::create_symlink("t.dbf", croot / ("linked" + std::string(ext)));
    }
    fs::create_directory_symlink("real", croot / "linkdir");
    {
        JailGuard jail({croot.string()});
        for (const char* ext : {".dbf", ".cdx", ".fpt", ".z01"}) {
            CHECK_FALSE(File::open((croot / ("linked" + std::string(ext))).string(),
                                   OpenMode::ReadOnly).has_value());
        }
        CHECK_FALSE(File::open((croot / "linkdir" / ".." / "t.dbf").string(),
                               OpenMode::ReadOnly).has_value());
        auto remounted = openads::platform::resolve_client_path(
            {croot.string()}, (croot / "linked.dbf").string());
        REQUIRE(remounted.has_value());
        CHECK_FALSE(File::open(*remounted, OpenMode::ReadOnly).has_value());
    }
    fs::remove_all(root);
}

TEST_CASE("data jail: symlinked configured root is refused") {
    const auto base = fresh_dir("openads_jail_root_link");
    fs::create_directories(base / "real");
    fs::create_directory_symlink("real", base / "linked");
    auto r = File::set_data_jail({(base / "linked").string()});
    CHECK_FALSE(r.has_value());
    File::clear_data_jail();
    fs::remove_all(base);
}

TEST_CASE("data jail cleared: legacy symlink follow returns for local use") {
    const fs::path base    = fresh_dir("openads_jail_local");
    const fs::path root    = base / "data";
    const fs::path outside = base / "outside";
    fs::create_directories(root);
    fs::create_directories(outside);
    const fs::path croot = canon(root);

    put_file(outside / "real.dbf", "local");
    fs::create_symlink(outside / "real.dbf", croot / "t.dbf");

    File::clear_data_jail();  // the ace client library never sets one
    CHECK(!File::data_jail_active());
    auto r = File::open((croot / "t.dbf").string(), OpenMode::ReadOnly);
    CHECK(r.has_value());
    fs::remove_all(base);
}

#endif // !_WIN32
