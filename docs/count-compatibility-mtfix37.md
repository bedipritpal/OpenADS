# mtfix37 count compatibility

Prepared for and tested with Pritpal Bedi on the bedipritpal/OpenADS fork of FiveTechSoft/OpenADS.

Default remote physical-count behavior returns to mtfix32: reuse the cached count, or the current navigation count tail, until the existing invalidation path clears it. This skips repeated count requests. Another workarea or local writer can change the physical count before the cached client notices. This is a compatibility/performance choice, not a claim of exact multiuser freshness.

Set `OPENADS_FRESH_COUNTS=1` in the client application environment before launching it to request a fresh server count on each count read. Leave it unset, or set it to `0`, for the default mtfix32 policy. This client setting does not need a server setting. The same old-server protocol is used.

Retained from mtfix34/35: no unused physical-count reads for ordered relative-position queries; corrected ordinary DBF size-based physical count; partial-encryption bitmap and full-encryption handling; disk refresh errors are propagated when a refresh is performed, without writing stale output. Default cache hits do not perform a disk refresh and cannot detect a later disk error.

The private mtfix36 operation-snapshot RDD proposal is not included. No RDD rebuild or application relink is needed for mtfix37. Replace client and server kits as usual, then test B_BIG followed by Vouch using the existing readiness procedure, with tracing off for timings. Fork main, upstream and live servers are not changed by publishing this test build.
