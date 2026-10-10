# MinGW static client

The implementation archive is `libopenace32.a` (32-bit) or
`libopenace64.a` (64-bit). `libace32.a`/`libace64.a` are DLL import
libraries and do not remove the ACE DLL dependency.

Use a matching MinGW Harbour build. Recompile the Harbour ADS driver
against the supplied `ace.h` declarations (cdecl for MinGW). Do not
mix an MSVC/stdcall ADS driver with the MinGW static archive.

Link the merged implementation archive after the Harbour ADS driver,
with the matching MinGW C++ runtime and Windows `ws2_32` and `psapi`
libraries. If the build tool groups static libraries, include it in that
group. Do not also add the DLL import library. Compile `oads_hb.c` into
the application for `OAds_SetLogging()` and the other OAds wrappers.
The diagnostics master starts OFF without an application call.

`tests/smoke/mingw_static_ace.cpp` is a packaging link/run gate against
the actual merged archive. `tests/smoke/harbour_static_ace.prg` supplies
a Harbour smoke program. A release must pass the Harbour smoke with
32-bit hbmk2 before claiming the user's Harbour kit is ready. This
candidate has not yet passed that hosted Harbour check.

Static linking removes the replaceable ACE DLL from this application.
It does not prevent executable patching, injected code or a rebuilt app.

Pinned Harbour validation uses source commit
`4ec2e154ed92757418bc30af571ab90240ab3209`. Its normal rddads.hbc
adds `ace32`/`ace64`, so the packaged static smoke compiles the four
stock RDD sources explicitly and links only the implementation archive.
`tools/scripts/test_harbour_static_mingw.sh` performs this check on
both Windows architectures and rejects any ACE DLL import.
