# OpenADS 1.09.72-mtfix33: physical count freshness

Test build on `feat/portal-health-mtfix30`. No fork-main or upstream changes.

## Integrity before speed

Remote `AdsGetRecordCount` on a table now refreshes the server's physical count on every call, matching Harbour DBFCDX's shared RecCount behavior. It no longer trusts an earlier client count or a navigation reply's in-memory count as proof that another workarea has not appended or removed records. Internal physical-count consumers refresh their snapshots too.

Exclusive handles are not exempt: this server permits sibling workareas on the same session, and a sibling can append while an exclusive handle remains open. No safe file-wide cache invalidation has been proven, so this build takes the fresh-count path there as well.

Index-handle scoped key counts, cursor navigation, record locks, durability, FileExists, and DBFCDX/Vouch source are unchanged. The wire protocol is unchanged. New clients work with older servers via the existing count request; older clients keep their previous cache behavior and need the new client to receive this correction.

## Cost and next step

This is an integrity correction, not a claimed speedup. Repeated count queries can now cost a WAN round trip where previous builds returned a stale snapshot. The prior mtfix32 trace still had35 actual count requests; that does not tell us how many additional requests this corrected build will issue in a readiness run. Measure readiness with tracing off and report the result honestly.

BOF/EOF/RecNo/Found/deleted already ride on most navigation replies. A fresh count piggyback alone cannot prove freshness at a later call after another client commits. A future speed change needs cross-session and local-DBF writer coherence, not a frozen count.

## Tests

Sandbox regression coverage includes another remote session's append, a local DBFCDX-side append and zap, and a same-session sibling append with an exclusive handle. Cursor independence is checked. A repeated shared-count test asserts that each call reaches the server rather than reusing a nav snapshot.

Local Debug tests:1633 cases /613727 assertions passed, excluding documented slow/flaky tests and the already-known exclusive-bag fixture mismatch. The create/index/append storm case failed with surplus CDX keys on both mtfix33 and unchanged mtfix32; it is not claimed fixed or newly introduced here. Optimized normal/TLS guard and platform packaging results are reported separately with the release.

## Credit

Pritpal Bedi owns this fork and supplied the Vouch logs, readiness runs, diagnostics, and the DBFCDX compatibility and integrity requirements. Based on FiveTechSoft/OpenADS. Private logs, paths, table names, and trace data are not included.
