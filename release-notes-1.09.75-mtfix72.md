# OpenADS mtfix72 cumulative test candidate
Pritpal Bedi. 7 November 2026.

Frozen71 plus capability-gated same-landing scoped key-count metadata for an ordinary ordered GoBottom whose internal key-position dependency is cold. One bottom reply instead of bottom plus GetKeyCount in that case. No Vouch speed claim. Existing matched71 commit fusion remains cumulative.

A 16-byte magic/status/count/table-id prefix wraps the otherwise unchanged row/boundary/pair reply only when requested by a capable new client. Successful count is consumed once to complete this GoBottom's last-key-position calculation. It does not seed persistent count caches. Natural order, warm count, existing fused-order and certified pair paths retain their existing behavior. Old peers retain B0 fallback.

No new commit, lock, unlock, first-field, RDD GoCold, on-disk format, Vouch source, new C, read-ahead or OPEN policy change. Ordered count uses the same ABI active order/scope/SET DELETED semantics as the prior internal B0 dependency. Failed optional count is not certified; legacy count path remains. Malformed/missing/wrong-table metadata fails this read without replaying navigation. Existing ignored navigation/fused-count return policies outside this new certification are not repaired or certified.

Explicit count APIs are not rewritten. Baseline71 already has persistent RemoteIndex key-count caches and optional physical-count caching unless OPENADS_FRESH_COUNTS=1. This candidate is not blanket freshness certification. Because the single-use GoBottom completion does not populate those caches, a later scoped count can now require its own request; it is not silently served from this new metadata. Existing empty-scope EOF/BOF distinctions remain and are tested separately, not normalized.

The owner's rule that record movement should return full table state is recorded as a design direction. This candidate supplies only the missing GoBottom count dependency. It does not implement a broad all-movement protocol overhaul or certify all table state/freshness. Production readiness and other open issues remain separate.

Net trips depend on the COMPLETE call sequence: bottom alone2->1; bottom followed by later explicit same-index count2->2(request shifts); bottom plus later explicit table-handle count3->2. The owner's captured final bottom has no later explicit count. No universal net-saving or elapsed-latency claim.
