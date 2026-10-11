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
Native required/optional protected-key-file daemon fixtures now PASS on x86/x64.
The fixture had an extra OWNER RIGHTS ACE; removing it fixed the fixture.
The production validator has not been weakened. Native TLS-on and lean TCP
size/export/timing comparison completed in run 38110622788. These are synthetic
loopback timings, not Vouch or WAN evidence.
The full suite is not green: slow timeouts and baseline index/duplicate-key
failures are separately recorded. Do not treat this source as a release.

Start reading include/openads/ace.h and src/abi/ace_exports.cpp for the API,
src/network/tls_transport.cpp for handshake policy, tools/serverd/main.cpp
for file protection and listener policy, and tests/unit/network_psk_test.cpp,
tests/smoke/tls_psk_daemon.py and tls_psk_adversarial_posix.cpp for proofs.

## Matched Winlibs10.2 measurements

TLS-capable TCP / PSK / lean TLS-off TCP medians (milliseconds):

| Architecture | Fresh connection (50) | 1000 small operations (5 runs) | 150 indexed prefetched rows (5 runs) |
|---|---|---|---|
| x86 | 0.6379 / 10.3567 / 0.6268 | 226.901 / 242.782 / 223.766 | 1.7966 / 2.0751 / 1.7768 |
| x64 | 0.5628 / 5.8502 / 0.5209 | 196.558 / 199.339 / 194.334 | 1.5952 / 1.6577 / 1.6429 |

Unstripped binary sizes (bytes), TLS-on / lean TLS-off:
x86 ACE 11,724,283 / 10,758,662; server 11,876,262 / 10,896,033.
x64 ACE 12,144,341 / 11,219,166; server 12,308,144 / 11,368,700.
Both DLLs export OAds_SetTlsPsk. No production release is implied.
Proof: https://github.com/bedipritpal/OpenADS/actions/runs/38110622788

The C API is exported; Harbour HB_FUNC glue is not yet bundled in this spike.
The supplied learning .prg example embeds a small C bridge. Do not call the
C export as a bare Harbour function without linking an appropriate bridge.
