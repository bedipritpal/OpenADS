# mtfix81: fresh single-pass case lookup

Cumulative80 with one optimization in resolve_case_insensitive only. A fresh
parent-directory scan remembers the first folded candidate but keeps looking
for exact spelling. An exact spelling wins. No match returns the input spelling.
Ambiguous folded candidates and enumeration-constructor errors use the original
two-pass algorithm. Iterator-increment exceptions retain the old error policy.
Unconvertible neighbours remain skipped. No cross-call cache or existence flag.

No change to refresh/twin logic, extension precedence, retries, locks, index
binding, cursor, memo, RDD, Vouch, wire format or existing packaging. No added
timers. The inherited80 numeric diagnostics remain OPEN80 and default OFF;
see open-phase80.md for the unchanged opt-in, cap and privacy rules.

Neither old nor new enumeration is an atomic snapshot during concurrent folder
mutation. The caller's existing current stat/open still determines availability.
Concurrent replacement tests check fresh admissible outcomes, no cached state
and server survival; they cannot prove identical scheduling-dependent results.

Tests: frozen80 reference versus new stable results/error codes, exact/folded/
missing, duplicate-case on case-sensitive filesystems, odd/unconvertible neighbor,
permission denial where host permits, fresh create/delete/recreate, concurrent
replacement and8readers. Native index and memo probes retain extension, mode,
error reply and memo-position coverage. Permission denial is explicitly labelled
UNAVAILABLE when the host cannot produce it, never counted as a pass.

Lookup-only benchmarks are fixture measurements, not customer open-time claims.
Delhi Windows and Delhi Linux old/new timings remain unmeasured until tested on
those installations. No claim of equal Windows/Linux production performance.
User deploys; no automatic service/config/auth/data or upstream changes.
