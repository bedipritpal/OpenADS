# OpenADS 1.09.72-mtfix37: mtfix32 count reuse by default

Test pre-release for Pritpal Bedi's bedipritpal/OpenADS fork, based on FiveTechSoft/OpenADS. Prepared at Pritpal's request. No private traces, source files or business data are included.

## Default behavior

Remote physical RecCount/LastRec returns to mtfix32's count reuse: a cached count or current navigation count tail can answer repeated calls without another server request. This also restores the old internal count-cache check. The existing invalidation paths are unchanged. Another station, sibling workarea or local writer can change N before a cached client notices; this is the selected compatibility/performance trade-off, not exact multiuser freshness.

## Optional fresh counts

Set `OPENADS_FRESH_COUNTS=1` in the client application's environment before launch for a fresh server count on each count read. Leave it unset, or set it to `0`, for mtfix32-style reuse. No server setting or protocol change is needed. Local physical counts still use the retained disk-refresh path.

## Kept from mtfix34 and mtfix35

- Ordered relative-position reads do not request a physical count they discard; compound target calculations avoid a second unused count.
- Ordinary DBF count uses physical file size rather than a stale high header. Full encryption remains size-based; partial-encryption bitmap tables keep their header path.
- Disk-refresh errors are returned without filling stale output when a refresh is performed. A cache hit does not perform a disk read and cannot detect a later disk error.
- Earlier direct-rightmost CDX GoBottom and all other mtfix35 features remain.

The private mtfix36 operation-snapshot RDD proposal is not included. No Vouch, Harbour RDD, Browse helper, fork-main or upstream source changes.

## Verification

Normal and TLS guard tests passed. Cross-platform CI passed all 11 jobs, including Windows x86/x64, macOS, TLS and Harbour smoke. Local focused policy/count tests passed 12 cases / 359 assertions. Non-slow/non-flaky sweep passed all three sequential partitions (285526, 29108 and 299362 assertions; boundary cases overlap). Tests explicitly cover default cached peer-stale behavior and opt-in fresh peer, sibling and local-writer visibility, refresh errors and the retained ordered query trim. All five packaged kits are checked before delivery.

## Use

Update the client and server with the matching test kits. Windows x86 is the 32-bit Vouch kit; standard Linux is for Ubuntu 24.04, linux-glibc231 for Ubuntu 20.04. No RDD library rebuild or application relink is required. With `OPENADS_FRESH_COUNTS` unset, test B_BIG first, then Vouch using the same readiness procedure with tracing off for timings. No speed result is claimed before those application runs.
