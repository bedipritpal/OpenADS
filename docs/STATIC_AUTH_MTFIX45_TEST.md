# 1.09.74-mtfix45-nodelay-test

TEST ONLY. Based on mtfix44 plus its Windows CRT fallback repair. No main merge.

Only performance change: TLS client TCP_NODELAY matches ordinary TCP. Auth stays on one connection, no new lane pool, same Oct12 TLS1.3/challenge/.hrb authentication and four worker slots. No server install is needed for the client socket change; matching Linux test kit is supplied separately. Use existing auth module, not a synthetic test .hrb.

Control baseline reported by owner: remote auth10-storm33.35s/1000 records, plain TCP10-storm about11s. Login try1, Begin about150ms, Complete by340-450ms. No retry/busy explanation for that run. No WAN speed-up claimed until measured.

Existing wire_trace was not installed by BeginAuth. This test build installs the same opt-in hook for auth too, excluding Begin/Complete payloads. With tracing OFF, compare elapsed times first. In separate traced runs, set OPENADS_WIRE_TRACE=1 and OPENADS_WIRE_TRACE_FILE to a different writable file for each run. One wire line per completed non-auth request/reply gives opcode, request/reply byte counts, duration and connection. No challenge, credential blob or field values are logged. Existing table/index names can appear. Tracing writes files and can slow a run; compare trace counts separately from clean speed tests.

Compare same exe/thread/record count, same server/data conditions, same URL path and one attempt per login. Record rows must match. Keep plain TCP lane config unchanged. Suggested sequence: mtfix44 auth, mtfix45 auth, mtfix44 auth again. Do not mix tracing-on time with tracing-off time. Compare request count/opcode totals and median/p95 request RTT; do not add overlapping thread RTTs and call that wall time.

Native transport proof measures1000 small-frame round trips on loopback, with real TLS and exact payload equality. It is a microbenchmark, not B_BIG/WAN or a row-integrity substitute. Existing DBF/CDX threaded and auth/integrity proofs also gate release.

Use matching libopenace32.a, librddads.a, ace.h and ads.ch. Same .hbp library lines as mtfix44, adjusted only to the extracted directory. Compiler/Qt unchanged. No separate oads_hb.c or compiler runtime libs. Check earlier Harbour library directories for stale shadow copies.
