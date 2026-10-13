# MinGW static client

The implementation archive is `libopenace32.a` (32-bit) or
`libopenace64.a` (64-bit). `libace32.a`/`libace64.a` are DLL import
libraries and do not remove the ACE DLL dependency.

Use a matching MinGW Harbour build. Recompile the Harbour ADS driver
against the supplied `ace.h` declarations (cdecl for MinGW). Do not
mix an MSVC/stdcall ADS driver with the MinGW static archive.

Link the packaged implementation archive after the matching Harbour ADS driver,
with Windows `ws2_32` and `psapi` libraries. The kit includes the matching
MinGW static C++/GCC/pthread runtime and compiled `oads_hb.c` wrappers. Do not
add a second runtime archive, compile another `oads_hb.c`, or add the DLL import
library. The Harbour VM and `librddads.a` remain part of your own Harbour build.
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

## Packaging profiles

`tools/scripts/package_static_client.py` packages runtime and wrapper objects
into one archive and writes a compiler/hash manifest. `matching` requires the
same compiler for runtime and wrapper objects. The release Harbour smoke uses
this path; it must pass before the packaged archive is staged for download.

`qt492-x86` is the owner-tested legacy profile: pinned i686 Winlibs GCC10.2
engine/runtime, unchanged Qt MinGW4.9.2 wrapper/link compiler. Only BigObj
containers are converted, not machine code. Its deferred TLS-exit bridge
registers cleanup in the normal CRT constructor phase instead of the early
loader callback, avoiding the old CRT exit-lock deadlock. It is executable-only
and must not be reused for unrelated DLLs or compilers. The owner test workflow
uses the same packager and checks concurrent DBF/CDX append integrity and worker
plus main-thread TLS cleanup.

Keep only one implementation archive visible in linker search directories.
The manifest names the exact compiler profile and output hash; a differently
named compiler installation is not a substitute for matching ABI/runtime.
A test build is not production approval or permission to merge it to main.
