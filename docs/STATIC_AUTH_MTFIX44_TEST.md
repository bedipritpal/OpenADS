# 1.09.74-mtfix44-static-auth-test

Owner test candidate, based on fork main 6d5c59f4. Not merged to main.

Only Oct12 auth behaviour is added to main's existing combined static client packaging. Existing main features and protections remain. No mtfix42 master diagnostics, mtfix43 wire padding, public-key pinning, PSK, mutual server proof or Vouch.dat design is added.

Auth uses BeginAuth/CompleteAuth with auth://host:port/path and the caller's existing VouchBuildCredentials. Transport remains encrypted but unverified TLS1.3. Server identity is not authenticated. Use the trusted application .hrb; synthetic probe modules are not for deployment.

Server: openads_serverd --config openads.ini --auth_module C:/absolute/vouch_auth.hrb
Keep openads_auth_worker.exe beside serverd. HTTP is not supported on this auth listener. Existing module ACL rules apply. Production-shaped test candidate keeps OPENADS_AUTH_CAPACITY_TEST_SWITCH=OFF, four simultaneous HRB worker slots. Remove auth_worker_concurrency from the INI for this kit; it is rejected with the switch OFF.

The qt492-x86 static archive includes engine, TLS dependencies, compiler runtimes and OAds Harbour wrappers. Use the supplied matching RDD archive and headers. Qt and the user's compiler remain unchanged.

HbIDE .hbp library lines, with the extracted kit at C:/OpenADS_1/build/mtfix44-static-auth:
-LC:/OpenADS_1/build/mtfix44-static-auth/lib
-lrddads
-lopenace32
-lws2_32
-lbcrypt
-lpsapi

Keep the project's existing Harbour/Qt/FiveWin settings. Add the supplied include path and ads.ch where needed. Remove any separate oads_hb.c, -lstdc++, -lgcc, -lgcc_eh, -lwinpthread or ACE DLL import library from the project. Avoid a second libopenace32.a/librddads.a in earlier Harbour search directories; hbmk2 may prepend them. Do not remove the project's own unrelated libraries.

After relinking, use the same Oct12 auth calls. Normal AdsConnect60 is not converted by relinking. Local/plain existing connect paths stay available as before. This is a new integrated test build; owner Vouch pass is required before main merge.
