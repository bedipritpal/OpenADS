// Real-daemon session regression probe: drives auth:// logins against a
// live openads_serverd process. Pattern: A login, B login concurrent,
// close A, C login. Prints one line per step with PASS/FAIL.
#include "openads/ace.h"
#include <cstdio>
#include <cstring>
#include <string>

static int connect_one(const std::string& url, ADSHANDLE& out) {
    ADSHANDLE pending = 0;
    UNSIGNED8 challenge[32];
    int rc = OAdsBeginAuth(url.c_str(), &pending, challenge);
    if (rc != 0) { std::printf("  BeginAuth rc=%d\n", rc); return rc; }
    std::string blob(reinterpret_cast<char*>(challenge), 32);
    blob += "test-only";
    rc = OAdsCompleteAuth(pending, reinterpret_cast<const UNSIGNED8*>(blob.data()),
                          static_cast<UNSIGNED32>(blob.size()), &out);
    if (rc != 0) std::printf("  CompleteAuth rc=%d\n", rc);
    return rc;
}

int main(int argc, char** argv) {
    if (argc != 2) { std::fprintf(stderr, "usage: %s auth://host:port/dir\n", argv[0]); return 2; }
    std::string url = argv[1];
    int fails = 0;
    ADSHANDLE a = 0, b = 0, c = 0;

    int ra = connect_one(url, a);
    std::printf("A login: %s\n", ra == 0 && a ? "PASS" : "FAIL");
    if (ra != 0 || !a) ++fails;

    int rb = connect_one(url, b);
    std::printf("B login (concurrent with A): %s\n", rb == 0 && b ? "PASS" : "FAIL");
    if (rb != 0 || !b) ++fails;

    if (a) { AdsDisconnect(a); a = 0; }
    std::printf("A closed\n");

    int rc2 = connect_one(url, c);
    std::printf("C login (after A closed): %s\n", rc2 == 0 && c ? "PASS" : "FAIL");
    if (rc2 != 0 || !c) ++fails;

    if (b) AdsDisconnect(b);
    if (c) AdsDisconnect(c);
    std::printf("%s\n", fails == 0 ? "ALL PASS" : "REGRESSION DETECTED");
    return fails == 0 ? 0 : 1;
}
