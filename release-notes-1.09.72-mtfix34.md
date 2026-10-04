# OpenADS 1.09.72-mtfix34: remove unused physical-count requests

Test pre-release for Pritpal Bedi's bedipritpal/OpenADS fork, based on FiveTechSoft/OpenADS. Pritpal's Vouch readiness runs, diagnostics, and integrity requirements guided this work. Private traces and business data are not included.

## Change

Ordered relative-position queries no longer request a physical table count which they immediately discard in favor of the active order's scoped key count. Ordered key navigation avoids the same unused request. Set-relative-position reuses the freshly fetched physical snapshot inside that one operation instead of reading it twice through nested helpers.

Public remote table RecCount/LastRec still refreshes on each call, as corrected in mtfix33. No sticky physical-count cache was restored, and no fresh public count is served from an earlier navigation reply. Same-session sibling and other-session/local-writer append/zap tests remain in place. No Vouch or Harbour RDD source changes, FileExists removal, durability changes, or fork-main/upstream changes.

## What this does not claim

This does not remove the public fresh-count requests made by Harbour's empty/EOF navigation path. It does not promise to recover the 13.459-second readiness difference between Pritpal's single mtfix32 and mtfix33 trace-off runs. The mtfix33 run had no opcode trace, so the frequency of the removed internal calls in that run is unknown.

The earlier direct-rightmost CDX GoBottom fix remains unchanged. CDX still descends the rightmost branches rather than walking all leaves from the top. Scoped/deleted exclusions and empty right-edge holes may require backward stepping; count setup around navigation is separate from that landing algorithm.

No wire protocol changes. Only the client DLL needs this change; old servers use the existing operations. Old clients keep their previous behavior.

## DBFCDX physical LastRec parity

Harbour's shared `hb_dbfRecCount` recomputes `floor((file size - DBF header length) / record length)` via `hb_dbfCalcRecCount`. Deleted records remain part of this physical count. The trailing 0x1A EOF byte is not explicitly subtracted; integer division discards it for ordinary record lengths. OpenADS GetRecordCount refreshes the CDX driver before replying and uses the same size formula, but takes the maximum of the header and size-derived counts. These agree for valid ordinary DBFs, including the EOF byte; a header overcount is a parity exception. Partially encrypted DBFs with a bitmap after the records use the header count instead because total file size includes that trailer. This build does not change those driver rules.

## Verification

Local Debug non-slow/non-flaky sweep: 1,635 cases / 613,813 assertions passed, with the known exclusive-bag fixture exclusion retained. Focused bottom/fresh-count/relative-position tests: 9 cases / 544 assertions passed. Synthetic actual DLL/server fixture: ten ordered relative-position queries sendzero physical count requests versus ten on mtfix33; one ordered set-position sends one versus two on mtfix33. Natural set-position sends one fresh physical count; a later peer append is visible on the next operation and on public RecCount.

Existing multilevel 20,000-key CDX bottom/peer-refresh and empty-tree/right-edge-hole regressions passed. The previously reported slow/flaky create/index/append CDX surplus-key issue and exclusive-bag 6106/7040 mismatch are not fixed here. Optimized normal/TLS guard, full CI and platform packaging are checked separately before delivery.

## Readiness test

Update the client DLL, run the same trace-OFF readiness procedure, and report the displayed time. Use the Windows x86 kit for 32-bit Vouch. Standard Linux kit is for Ubuntu 24.04; Ubuntu 20.04 uses linux-glibc231. Do not mix traced and untraced timings.
