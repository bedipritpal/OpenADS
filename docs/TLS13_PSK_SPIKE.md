# TLS1.3 external PSK spike - UNMERGED, NOT FOR PRODUCTION

Prepared for Pritpal Bedi. Based on mtfix42; independent of mtfix43 padding.
No main merge, upstream PR or crypto release is authorized by this branch.

Client: OAds_SetTlsPsk(identity, key, 32) before AdsConnect60 on the same thread.
Identity is a NUL-terminated 1..128-byte string; key is 32 raw secret bytes.
Connect using tls://host:port/data-directory and ordinary database credentials.
Clear future-connection settings with OAds_SetTlsPsk(NULL,NULL,0).
New connections snapshot settings; existing sessions do not change on clear.

Server: tls_mode=required and tls_psk_file=/protected/path/keys.
Each line: identity:64_hex_digits. Use 32 cryptographically random key bytes,
never the public deterministic test fixtures. Duplicate/malformed entries fail.
optional mode requires a distinct explicit plaintext_port for migration.
There is no same-port plaintext fallback. off rejects supplied TLS credentials.
PSK and certificate configuration together are rejected as ambiguous.

TLS uses mbedTLS 3.6.2, TLS1.3 external PSK plus ephemeral key exchange only.
Session tickets are disabled; 0RTT is disabled by the dependency configuration.
PSK transport authentication does not replace database user/permission checks.
Secrets are not put in URIs/logs; snapshots are wiped on clear/destruction.
Protected key files are checked and read through one descriptor/handle.

Status at publication: focused native x86/x64 client proofs pass 21 assertions
per architecture. POSIX tamper/replay probes reject the affected session and
permit the next legitimate session. Selected local network tests passed.
Windows valid owner-only file daemon startup is STILL FAILING the ACL gate.
The gate has not been weakened. Native size/lean TCP comparison is unfinished;
handshake timings are preliminary synthetic evidence, not Vouch timings.
The full suite is not green: slow timeouts and baseline index/duplicate-key
failures are separately recorded. Do not treat this source as a release.

Start reading include/openads/ace.h and src/abi/ace_exports.cpp for the API,
src/network/tls_transport.cpp for handshake policy, tools/serverd/main.cpp
for file protection and listener policy, and tests/unit/network_psk_test.cpp,
tests/smoke/tls_psk_daemon.py and tls_psk_adversarial_posix.cpp for proofs.
