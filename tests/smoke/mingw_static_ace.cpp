// Link this against the actual merged archive, not openads_core or a DLL import lib.
#include "openads/ace.h"
#include <cstdio>
extern "C" int oads_logging_enabled(void);
extern "C" void oads_set_logging(int);
int main() {
    if (oads_logging_enabled() != 0) return 1;
    OAdsSetLogging(1);
    if (oads_logging_enabled() != 1) return 2;
    OAdsSetLogging(0);
    UNSIGNED8 folder[] = ".";
    ADSHANDLE connection = 0;
    if (AdsConnect60(folder, ADS_LOCAL_SERVER, nullptr, nullptr, 0, &connection) != 0) return 3;
    if (AdsDisconnect(connection) != 0) return 4;
    std::puts("merged static ACE link/run OK");
    return 0;
}
