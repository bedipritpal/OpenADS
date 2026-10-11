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

## Required compatibility baseline (Pritpal Bedi, October 10, 2026)

Future static kits must use the pinned Winlibs GCC10.2.0 r8 toolchains, mingw-w64 8 / MSVCRT: x86 posix-dwarf and x64 posix-SEH. GCC's `_GLIBCXX_HAVE_TLS=1` and emutls once helpers are part of the compatibility contract. This is compiler-runtime TLS, not OpenADS network TLS; `OPENADS_WITH_TLS=OFF` remains the lean client setting.

Do not substitute rolling MSYS2 GCC or Ubuntu GCC10.3 TLS-off builds. They do not match the owner's runtime. No compiler upgrade or Qt rebuild is required on his machine. Ship the stock rebuilt RDDADS archive and `ads.ch` alongside static ACE. Without `rddads.hbc`, use `-u+<path>/ads.ch` in the hbp or explicitly include `ads.ch` so PRG constants remain defined.

The release workflow pins download digests and rejects wrong GCC versions, missing TLS/emutls macros, incompatible once/time32 symbols and ACE/C++/winpthread DLL imports. Native Windows normal and whole-archive static link/run are mandatory. Keep toolchain and symbol proof in each Windows release package. The auxiliary MSYS2 shell is not a source of compiler-runtime replacements.

The dedicated `.github/workflows/mtfix42-gcc10-static.yml` recipe remains an exact-mtfix42 reproduction; its fixed source checkout is intentional. The rolling release recipe builds the source being released with the same matched toolchains. Existing published mtfix42 assets are not changed by these process edits. Main-branch adoption remains subject to the fork's existing owner-clearance rule.
