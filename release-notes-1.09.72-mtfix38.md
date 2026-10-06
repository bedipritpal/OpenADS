# OpenADS 1.09.72-mtfix38: parent reconciliation test build

Prepared for Pritpal Bedi's bedipritpal/OpenADS fork, based on FiveTechSoft/OpenADS. Thanks to Pritpal for the B_BIG and Vouch timings, logs, and regression testing that guide this work. No private application source, traces or business data are included.

This test branch reconciles fork mtfix37 (cca52c2) with parent main (91a63f6). It is not a merge to fork main and is not an upstream submission. Pritpal's B_BIG and Vouch checks are required before a main merge.

## Retained fork behavior

- mtfix32-style physical count reuse remains the default. OPENADS_FRESH_COUNTS=1 selects fresh count reads.
- Reused counts can miss a peer append or a later disk error until normal invalidation. Fresh mode avoids that reuse.
- mtfix35 size-only count refresh and error propagation, mtfix34 ordered count trimming, and mtfix37 count tests remain.
- Health/portal/helper support, x86 stdcall imports, five-platform kits, and credits remain. Private mtfix36 snapshot work is held out. Vouch and Browse-helper are unchanged.

## Parent work and conflict decisions

- Bring in parent's SQL, dictionary permission, execution-budget, archive and engine limits with both sides' tests.
- Keep hardened daemon wire validation, management authentication, TLS, login throttling and jailed file handling. Legacy embedded transport behavior stays separate from daemon mode.
- Shared client decoders keep legacy unknown-opcode behavior; strict daemon FrameReader checks reject bad headers early.
- Management security tests explicitly use hardened daemon mode, matching serverd. Fork health management handshake and TCP endpoint support remain.
- Retain reviewed five-kit packaging and manual test-release safeguards. Add parent's explicit repack-only job without invoking it or changing any existing release.

## Validation

Local Linux TLS-off and TLS-on unit targets built with one worker and debug symbols disabled for the sandbox's memory limit. Normal tests were split by test-number ranges because a full foreground run exceeds the execution window. Count and ordered-prefetch checks passed; other CTest items passed. Remote platform CI and final kit inspection must pass before publication.

## Pritpal's next check

Use the matching kit in a sandbox, reopen your application as usual, then compare B_BIG and Vouch readiness with mtfix37. Ubuntu 24.04 uses linux-x64; Ubuntu 20.04 uses linux-x64-glibc231. Rebuild and relink applications that embed the static RDD library if using the new RDD code.
