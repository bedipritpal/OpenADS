# Owner directives for this fork (bedipritpal/OpenADS)

Standing rules from the owner, Pritpal Bedi. Read before any change. These survive memory resets.

## Vouch and compatibility
1. NO modifications in Vouch, ever. Vouch is a real-time production ERP, running on DBFCDX for 33 years with zero errors. Fixes go on the RDD / OpenADS side only.
2. Vouch is used to keep DBFCDX and ADSCDX compatible at all times, loopback or remote. ADSCDX must behave like Harbour DBFCDX. Where OpenADS and DBFCDX differ, follow Harbour's code.
3. Do not build fixes around Vouch habits. Fix general RDD behavior correctly.
4. No field-length caps anywhere (1k-length fields exist).
5. Vouch builds are 32-bit. The x86 stdcall exports must keep working.

## Locks
6. Fail immediately, only when the application asks. DBFCDX/Clipper contract, CacheRDD is the reference: a repeat lock returns .T., one unlock frees it, status means "locked by me".

## Priorities
7. Integrity beats speed. A speedup counts only when it matches upstream 1.09.69 integrity.
8. Speed is the top priority after integrity, especially Vouch time-to-readiness over the WAN.
9. 1.x is stability. Shared physical opens (one real open per file for many clients, LetoDB-style) belong to 2.x.
10. ADSCDX data must never follow symlinks.

## Process
11. Fork test builds (mtfixN pre-releases on the test branch) go out without asking. Merges to fork main need the owner's test clearance with Vouch running many instances and threads. Credit the owner and his logs, tests and diagnostics by name in notes.
12. Nothing goes upstream (FiveTechSoft/OpenADS) without showing the owner the exact content first. The owner clicks "Create pull request" himself.
13. The owner's GitHub login is for bedipritpal/OpenADS only.
14. Never put passwords, tokens or private trace data in the repo or in chat. Traces contain table names and paths: private.
15. Keep the repo in step with upstream by syncing, not hand-porting.

## Static client release toolchain
16. All future static client kits and release packages must include matched Winlibs GCC 10.2.0 static ACE and stock RDDADS archives: i686-posix-dwarf for x86, x86_64-posix-SEH for x64, mingw-w64 8 / MSVCRT with _GLIBCXX_HAVE_TLS=1 and the emutls __once_call/__once_callable runtime family. Pin and checksum-verify the compiler downloads. Do not substitute rolling MSYS2 GCC or Ubuntu GCC10.3 TLS-off archives. Do not require a compiler upgrade or Qt rebuild on the owner's machine.
17. Before shipping, require normal and whole-archive static link/run on native Windows, symbol-family validation and no ACE/C++/winpthread DLL imports in the static proof executable. Ship the corresponding rebuilt stock RDD archive and matching headers. Do not carry the superseded Ubuntu TLS-destructor repair object into the matched Winlibs kit unless a new independently reproduced failure requires it.

Source: Pritpal Bedi, October 10, 2026: "Make sure now onwards builds have these libs." This names the TLS/emutls-compatible Winlibs10.2 replacement kits delivered immediately before the directive; exact compiler release revision on his machine was not identified. Verified build recipe: https://github.com/bedipritpal/OpenADS/actions/runs/38097149118. Existing published packages are not retroactively changed by this directive.
