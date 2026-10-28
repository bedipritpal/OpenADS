// Native ACE mirror of the owner-supplied deleted-only scope repro.
// Synthetic rows only. No application or customer data is needed.
#include "doctest.h"
#include "openads/ace.h"
#include "network/server.h"
#include <filesystem>
#include <string>

namespace {
UNSIGNED8* bytes(std::string& value) {
    return reinterpret_cast<UNSIGNED8*>(value.data());
}
void success(UNSIGNED32 status) { REQUIRE(status == 0); }
}

TEST_CASE("Tim 2e deleted-only scope never substitutes a preceding live row") {
    namespace fs = std::filesystem;
    const auto dir = fs::temp_directory_path() / "openads_tim_2e_deleted_scope";
    fs::remove_all(dir);
    fs::create_directories(dir);
    ADSHANDLE connection = 0, table = 0, index = 0;
    std::string root = dir.string(), name = "T_REC.dbf";
    std::string definition = "WRKORD,C,6,0;LABCOD,C,15,0";
    success(AdsConnect60(bytes(root), ADS_LOCAL_SERVER, nullptr, nullptr, 0, &connection));
    success(AdsCreateTable(connection, bytes(name), nullptr, ADS_CDX, ADS_ANSI,
                           0, 0, 0, bytes(definition), &table));
    std::string field = "WRKORD", label = "LABCOD";
    unsigned row = 0;
    for (std::string key : {"105555", "105556", "105557", "105557", "105566"}) {
        ++row;
        success(AdsAppendRecord(table));
        success(AdsSetString(table, bytes(field), bytes(key), 6));
        std::string value = "C" + key + "_" + std::to_string(row);
        success(AdsSetString(table, bytes(label), bytes(value),
                             static_cast<UNSIGNED32>(value.size())));
        if (row == 3 || row == 4) success(AdsDeleteRecord(table));
        success(AdsWriteRecord(table));
    }
    std::string bag = (dir / "T_REC.cdx").string(), tag = "WRKORD", expression = "WRKORD";
    success(AdsCreateIndex61(table, bytes(bag), bytes(tag), bytes(expression),
                            nullptr, nullptr, ADS_COMPOUND, 512, &index));
    success(AdsCloseTable(table));
    success(AdsDisconnect(connection));

    for (bool remote : {false, true}) {
        INFO("remote=" << remote);
        openads::network::Server server;
        server.set_daemon_hardening(true);
        if (remote) REQUIRE(server.start("127.0.0.1", 0));
        std::string uri = remote ? "tcp://127.0.0.1:" + std::to_string(server.port()) +
                                  "/" + dir.generic_string() : root;
        success(AdsConnect60(bytes(uri), remote ? ADS_REMOTE_SERVER : ADS_LOCAL_SERVER,
                             nullptr, nullptr, 0, &connection));
        success(AdsShowDeleted(0));
        success(AdsOpenTable(connection, bytes(name), nullptr, ADS_CDX, ADS_ANSI,
                             ADS_COMPATIBLE_LOCKING, ADS_IGNORERIGHTS, ADS_SHARED, &table));
        success(AdsGetIndexHandleByOrder(table, 1, &index));
        for (std::string key : {"105557", "105560", "105556", "105557"}) {
            INFO("scope=" << key);
            success(AdsSetScope(index, ADS_TOP, bytes(key), 6, ADS_STRINGKEY));
            success(AdsSetScope(index, ADS_BOTTOM, bytes(key), 6, ADS_STRINGKEY));
            const bool visible = key == "105556";
            auto check_landing = [&] {
                UNSIGNED16 eof = 0;
                UNSIGNED32 recno = 0, length = 63;
                UNSIGNED8 value[64] = {};
                success(AdsAtEOF(table, &eof));
                success(AdsGetRecordNum(table, 0, &recno));
                const auto status = AdsGetString(table, bytes(field), value, &length, ADS_NONE);
                CHECK((status == 0 || (!visible && status == 5068)));
                CHECK(eof == (visible ? 0 : 1));
                CHECK(recno == (visible ? 2 : 6));
                CHECK(std::string(reinterpret_cast<char*>(value)) == (visible ? "105556" : ""));
            };
            success(AdsGotoTop(index));
            check_landing();
            // Repeated navigation and the opposite-boundary pair must retain truth.
            success(AdsGotoTop(index));
            check_landing();
            success(AdsGotoBottom(index));
            check_landing();
            success(AdsGotoTop(index));
            check_landing();
            success(AdsSkip(index, 1));
            UNSIGNED16 eof = 0;
            success(AdsAtEOF(table, &eof));
            CHECK(eof == 1);
        }
        success(AdsCloseTable(table));
        success(AdsDisconnect(connection));
        server.stop();
    }
    fs::remove_all(dir);
}
