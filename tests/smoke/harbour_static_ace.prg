/* Build with 32-bit MinGW Harbour and the merged libopenace32.a only.
   Supply oads_hb.c and matching ACE includes; do not add libace32.a. */
#include "ads.ch"
REQUEST ADS
PROCEDURE Main()
   LOCAL hConn
   OAds_SetLogging( .F. )
   AdsSetServerType( ADS_LOCAL_SERVER )
   hConn := AdsConnect( "." )
   IF Empty( hConn )
      ErrorLevel( 1 )
      RETURN
   ENDIF
   AdsDisconnect()
   ? "Harbour static ACE connect/disconnect OK"
RETURN
