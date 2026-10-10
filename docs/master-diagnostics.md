# Master diagnostic output

Client libraries start with diagnostic output OFF. `OAdsSetLogging(1)` in C,
or `OAds_SetLogging(.T.)` through the matching Harbour OAds glue, permits
output. `OAdsSetLogging(0)` / `OAds_SetLogging(.F.)` silences it again.
No environment variable or client INI value enables this master switch.
Once enabled, each existing diagnostic setting retains its own behavior.

The switch is process-wide per loaded library image, across threads and
connections. It covers wire trace, ARC C++/x86 wrapper trace, table/index
creation diagnostics, CDX lock diagnostics, audit/resolve output, general
logging streams, core stderr diagnostics and persistent error-log output.
Disabling waits for current diagnostic writers; old files are not deleted.
Errors still return to the caller. Database transaction journals, recovery
maps, data/index files and backups are unaffected.

The standalone daemon starts with core diagnostics OFF too. Its operator can
set `diagnostics = 1` in the server INI or pass `--diagnostics 1`; explicit
`--diagnostics 0` overrides the INI. Invalid values are rejected. Existing
individual environment settings still select which traces run once the
master is enabled. The daemon's startup, TLS warnings and fatal operator
messages stay visible. Protect the server INI and launch command with OS
permissions: this is an operator setting, not remote per-user authorization.
A client's master switch does not alter a separate server process.

This prevents environment/INI-only activation of diagnostics. It is not an
anti-debugging boundary against somebody who can replace or inject code
inside an application they control. Keep logs private during approved support
tests; turning the master OFF does not erase traces written earlier.

## Masking while enabled

Diagnostic table/file fields use opaque labels, not reversible hashes.
WAN labels are connection-scoped and reset on a master-switch epoch change;
explicit application aliases remain visible. The label map is not written.
CreateTable, index, lock, resolve/audit and error-file output also mask paths.
Index expression/condition/filter text is omitted. Persistent error files
keep codes, stages and context but replace arbitrary detail with
`<detail-masked>`; API errors still return their original details.
Headless `AdsShowError` emits a fixed masked marker when enabled, never the caller text.
Existing files are not scrubbed. Protocol payloads are unchanged. This is
log masking, not traffic encryption or protocol-name masking.
