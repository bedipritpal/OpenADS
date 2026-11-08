# mtfix71 cumulative record-add test candidate

2026-11-07 18:40 CST. Credit: Pritpal Bedi, whose multi-instance/thread tests established the baseline to protect.

Based on frozen mtfix70, retaining its diagnostics and every earlier cumulative fix, including the mtfix52 line. No Vouch source changes. No new C code, exported ACE signatures or on-disk format changes. Test-only, not production clearance.

## Change

At an existing remote AdsWriteRecord boundary, a qualifying pending multi-field batch can carry its commit in the same SetFields frame. The first field still goes immediately, as in70. The remaining ordered fields then use one capability-gated packet instead of SetFields followed by FlushTable. This removes one round trip per qualifying commit, not one per field or a measured whole-Vouch saving.

The server uses the exact existing FlushTable twin-before-engine work after all fields succeed. On field failure, it stops at the same ordered prefix and does not flush. It does not roll back that prefix. Each explicit commit remains separate, including a key-only commit followed by a whole-record commit. Normal RDD GoCold/movement boundaries remain unchanged.

No fusion when a cursor settlement, deferred order or teardown operation would intervene. Single-field and clean commits retain their existing paths. Locks, fresh counts, OPEN, SetOrder and read-ahead behavior are not changed.

## Compatibility and errors

New server advertises kCapSetFieldsCommit (0x1000). New client sends a one-byte commit trailer only after that advertisement. A successful fused commit must return SetFieldsAck payload {1}. An empty or malformed acknowledgement is not proof of commit: the connection is closed, an error is returned and the client does not replay writes. Existing lost-reply handling also closes the connection. Explicit server error codes are preserved.

Older clients send ordinary SetFields to the new server. The new client retains SetFields then FlushTable on older servers. Both sides need71 to get this saving; a DLL-only upgrade against68/70 retains existing behavior.

Existing first-field/coalesced-set error timing limitations and the existing ignored ABI-twin flush return remain outside this narrowly scoped packet change. This candidate does not claim to repair or certify those paths.

## Verification requirements

Keep all previous platform/compiler, native baseline, explicit-refresh, Tim2e, append-metadata, auth, diagnostics/privacy and package gates. Add commit-boundary tests for exact two-commit disk state, index ordering, long fields, lock ownership, failed-prefix behavior, missing/malformed/lost acknowledgements and old/new version mixes. Local generated-data tests are not a substitute for the owner's Vouch exercise.

No automatic deployment, main merge or upstream submission. Old releases/assets remain frozen.
