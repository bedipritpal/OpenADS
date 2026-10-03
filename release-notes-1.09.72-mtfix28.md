# OpenADS 1.09.72-mtfix28 test build

Fork: bedipritpal/OpenADS, from FiveTechSoft/OpenADS.

Pritpal Bedi set the serverd-only security boundary and delegated security decisions while keeping RDD behavior and speed his top priority. He approved the one narrow exception: remote AdsMgConnect now sends its documented password. AdsConnect60 and table/RDD client paths are not changed. His B_BIG scaling runs, Vouch boot timing and HbDBU mixed-RDD diagnostics remain the application regression baseline, not security validation.

## Selective upstream security sync

- Daemon-activated connection state, management credential checks, login throttling and bounded session resources. Embedded/LOCAL defaults remain unchanged.
- Native server TLS termination when a TLS-enabled kit is started with both --tls_cert and --tls_key. No plaintext fallback on that listener. Existing client TLS behavior is retained.
- Server credentials are held as salted PBKDF2 verifiers. No DD password migration, local ACL policy change or at-rest format change is imported.
- Remote management authenticates on every probe, snapshot and mutator socket. Without daemon credentials, literal loopback management is read-only; mutators require a configured daemon administrator.
- Non-loopback anonymous daemon startup is refused unless --allow_anonymous is explicit. The daemon now defaults to 127.0.0.1. Existing network deployments must explicitly choose their bind address and authentication settings.
- Authenticated idle RDD sessions are not disconnected after five minutes. Unauthenticated, incomplete-frame and blocked-response timeouts remain.

## Deployment cautions

The management administrator is a daemon auth_user, not a migrated DD account. Configure matching daemon credentials for AdsMgConnect. Non-TLS management sockets still carry passwords in cleartext: use a trusted/firewalled network or a TLS proxy. Native TLS does not turn Studio HTTP into HTTPS and does not add tls:// support to AdsMgConnect. Use the existing external TLS termination path for those services.

The first-milestone resource limits can reject unusually large sessions: 256 open tables/cursors, 4096 record locks, 64 created mutexes, bounded field counts and fetch output/scan work. Normal RDD regression tests pass, but this is not a claim that every large deployment fits these limits.

mtfix27 table/companion no-follow jail is preserved. Generic file functions, ZIP/backup paths, full parser hardening and at-rest authentication remain separate work. No upstream PR is opened by this build.

## Validation

Local TLS and non-TLS builds pass full ctest 6/6. Regression tests cover remote management credentials on every socket, failed login, loopback read-only management, LOCAL management, unchanged AdsConnect60 table reads, native TLS certificate/hostname checks and a stalled-handshake single-worker test. Guarded strict clang TLS/non-TLS checks pass for the staged code. Final cross-platform packaging and archive verification must finish before this test release is published.

The bundled TLS loopback key is a public upstream test fixture, never a deployment credential. Only its exact scanner fingerprint is excluded; the private-key rule remains enabled.
