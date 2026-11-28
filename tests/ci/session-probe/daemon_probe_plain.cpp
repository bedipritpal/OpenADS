// Real-daemon session regression probe, plain TCP auth variant: drives
// AdsConnect60 user/password logins against a live openads_serverd started
// with --auth_user (no --auth_module, no HRB worker). Pattern: A login,
// B login concurrent, close A, C login. Prints one line per step PASS/FAIL.
#include "openads/ace.h"
#include <cstdio>
#include <cstring>

static int connect_one(const char* url, ADSHANDLE& out) {
    UNSIGNED32 rc = AdsConnect60((UNSIGNED8*)url, ADS_REMOTE_SERVER,
                                 (UNSIGNED8*)"usr", (UNSIGNED8*)"pw", 0, &out);
    if (rc != 0) std::printf("  AdsConnect60 rc=%u\n", (unsigned)rc);
    return (int)rc;
}

int main(int argc, char** argv) {
    if (argc != 2) { std::fprintf(stderr, "usage: %s tcp://host:port/dir\n", argv[0]); return 2; }
    const char* url = argv[1];
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
