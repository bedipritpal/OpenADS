#include "hbapi.h"
#include "ace.h"
HB_FUNC(COPY86CONNECT) {
    ADSHANDLE h=0;
    UNSIGNED32 rc=AdsConnect60((UNSIGNED8*)hb_parc(1),ADS_REMOTE_SERVER,0,0,0,&h);
    hb_retnint(rc?-(HB_MAXINT)rc:h);
}
HB_FUNC(COPY86DISCONNECT) { hb_retnint(AdsDisconnect((ADSHANDLE)hb_parnint(1))); }
