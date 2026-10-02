Built for Pritpal Bedi - bedipritpal/OpenADS, forked from FiveTechSoft/OpenADS.

# v1.09.72-mtfix24 - macOS sync test build

Test build only, not a production recommendation. Use matching client/server kits and back up binaries and data before testing. Fork main is unchanged; Tim Stone's exact ECLMST application test remains its merge gate.

## Upstream sync

An ancestry-preserving no-fast-forward merge takes the exact upstream v1.09.72 tag f464c91cd4ac337cd66daa125ec162cd2fe8ec46, not later upstream main. All mtfix23 ABI, engine, network, driver and public-header code is retained. Upstream already imported those fork changes through PR #178. The new engine changes are limited to platform/file_posix.cpp and platform/lock_posix.cpp.

On macOS the table-open share guard now uses an OFD fcntl sentinel-byte lock instead of flock, avoiding the collision with record/append locks. Record/append byte locks use process-scoped fcntl locks on macOS; blocking acquisition is bounded at 30 seconds with errno/holder diagnostics on timeout. The fallback also applies to POSIX platforms without OFD locks. Normal Linux-OFD and Windows locking paths are unchanged.

## macOS limitations and release gate

Thirteen same-process two-handle lock tests have macOS-only doctest::may_fail annotations across eight files. They still run, but their failures do not fail that suite. The hang fix does not restore per-handle record-lock conflicts on macOS; that remains upstream follow-up work. Cross-process locking and the OFD share guard are separate from this limitation.

The fork's real macOS CI gate is retained. Packaging no longer treats macOS failures or timeouts as optional: either now blocks release. The actual macOS test-step result and duration must be checked before publication. Upstream tag CI completed its macOS Test step in 102 seconds; this is upstream evidence, not a result for this fork build.

## Packaging and diagnostics retained

All five kits remain required for review: Windows x64/x86, Linux x64, Linux glibc 2.31, and macOS universal. Each includes include/ace.h and include/openads/ace.h with companion headers. The fork keeps its header-copy steps, glibc231 build, draft/source_ref staging, archive-opening/CRC/header/version/digest checks, mtfix tag-push skips, immutable public mtfix releases and secret scanning. Upstream's restored release workflow is not imported; its current 1.09.72 archives lack those headers.

OPENADS_LOCK_DIAG remains opt-in: set it to a writable filename to collect CDX lock/open/error diagnostics; unset or empty creates no routine diagnostic file. No Vouch application-code change is included.

## Validation before packaging

Local Linux incremental Debug -O0/-g0 build passed with inherited warnings-as-errors disabled. The complete six-entry CTest suite passed, including normal slow/flaky exclusions, slow cases, 30-repeat pool stress and SQL/SQLite checks. This is not a fresh multi-platform build. Fresh Linux/Windows/macOS packaging gates must pass separately.

Known full-enabled-suite baseline failures from mtfix23 remain unresolved: Exclusive-held OpenIndex may return 6106 instead of 7040, and the REMOTE create/index storm can have a count mismatch. Normal CTest excludes the flagged flaky cases; its pass is not a full-enabled-suite pass. No Tim ECLMST application pass is claimed.
