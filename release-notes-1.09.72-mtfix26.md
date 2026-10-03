Built for Pritpal Bedi - bedipritpal/OpenADS, forked from FiveTechSoft/OpenADS.

# v1.09.72-mtfix26 - CDX bottom navigation test build

Test build only, not a production recommendation. Use matching client/server kits and back up binaries and data before testing. Fork main and upstream are unchanged by this test build. Pritpal Bedi's application testing is the merge gate.

## Diagnosis and credit

Pritpal Bedi supplied B_BIG and captured the mtfix25 loopback client wire trace that located the delay: 100 AppendBlank requests across 10 workareas were individually fast, but the 10 initial ordered GoTop requests took 386-849ms each. GoTop's boundary-pair reply also prepares GoBottom on the server. CDX bottom positioning started at the first leaf and decoded all right-linked leaves, so a seemingly small initial navigation did linear index work. Other workers' setup and same-lane queuing stretched the interval between the first and last inserted-row timestamps to about 2.2 seconds.

The earlier B_BIG fix removed eager key counts. This trace has no GetKeyCount/GetKeyNum request and no fused-order GoTop, but the opposite-boundary leaf walk remained. This explains the present trace; the specific historical mtfix20-to-mtfix21 timing change is not established.

## What changed

CDX seek_last now follows the final child in each branch to the rightmost leaf. Empty tail leaves left by erases are skipped toward the left. Peer-header refresh is retained, including a changed root. Branch size/depth checks return a corruption error rather than reading outside a page or following a branch cycle indefinitely. Leaf encoding, ordering, scopes, deleted-record filtering, locks and writes are unchanged. No B_BIG or Vouch application change is included.

Regression tests cover empty indexes, multi-level duplicate-key trees, a peer append, empty tail leaves, backward positioning and an entirely erased tree. Existing scope, descending order and boundary-pair tests run in the full suite.

## Local validation

All six Linux Debug CTest entries passed, including slow tests, 30-repeat pool stress and SQL/SQLite checks. The 13 targeted CDX cases passed 3674 assertions. Two first-run SQL assertions were caused by inherited stale build objects after mtfix25; recompilation made both pass without unrelated source edits.

A scratch synthetic 1.3M-key index probe changed seek_last from roughly 133-155ms to 0.007-0.012ms locally. These are not Windows or B_BIG application timings and are not a promise of end-to-end speed. Fresh Windows/Linux/macOS packaging gates and archive inspection are required before publication.

## What to test

Repeat B_BIG on loopback with matching mtfix26 client and server, the same table and lane settings, and compare inserted-row timestamps. Capture a trace if delay remains. Also test ordered GoBottom, backward browsing, scope edges, descending orders and concurrent appends in the real application. The prior platform and ADT limitations remain unchanged. No new application pass is claimed.
