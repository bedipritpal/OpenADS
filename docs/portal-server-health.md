# Portal server health (mtfix30 test)

Requested by Pritpal Bedi. Fork bedipritpal/OpenADS from FiveTechSoft/OpenADS.
The same schema1 health statistics returned by OAds_ServerStats are shown in:
- Embedded Studio: Server tab, manual Refresh server health. API route:
  /api/server/health under the existing HTTP authentication gate.
- DA-Web: Server Info, above the existing user/table/query lists. Manual Refresh.

Every scalar and usage entry is displayed, with Current/Max used/Rejected.
Unknown values say Unavailable/not measured, never false 0. Expand Count meanings
and raw JSON for the exact schema/semantics. No new polling timer was added.
Counts remain best-effort concurrent samples; different queries can differ.
Workarea/table-handle maxima now retain each open/close event, not just the
current query value. Other max fields retain their existing telemetry semantics.
Workareas are server-open handles including parked ones, not OS descriptors
or Harbour Select areas. Distinct path counts are separate.

## DA-Web management login

Daemon management credentials are separate from DD credentials. A hardened
remote daemon prompts for management login. DA-Web never reuses DD username
or password. Successful management credentials are held only in the server-side
PHP session, scoped to the dictionary's endpoint, for 20 minutes. Forget management
login removes them; DD disconnect also clears them. Endpoint changes/expiry
invalidate them. Password is cleared from the browser field immediately after
submit; it is not placed in URL, localStorage, logs or returned JSON.
Use HTTPS to enter credentials (literal loopback is allowed for development).
Login/logout require a session CSRF token and JSON POST. The API requires the
selected DD connection to exist in this session. This is not a new portal-wide
login system or a claim that the existing PHP session framework is hardened.

AdsMgConnect itself still sends credentials on its raw management socket.
Use a trusted/firewalled management route or the existing TLS proxy. Native
server TLS does NOT add tls:// management support. Studio HTTP is not HTTPS.
No authentication gate or daemon policy is bypassed. If no daemon credentials
are configured, its existing literal-loopback read-only exception is retained.
Older DLL/server pairs report unavailable rather than fabricated statistics.

Install the matching mtfix30 DLL/server. DA-Web is not included in the binary
kits: deploy the DA-Web files from this test branch to your PHP portal separately.
The FFI declarations in server_info.php use the actual 32-bit ADSHANDLE ABI,
including when the PHP process is 64-bit. All normal RDD/ACE interfaces remain.

Local verification uses sandbox fixtures, not a user's server. Test build only;
fork main merge awaits application test clearance, no upstream submission.

## Harbour convenience call

`OAds_ServerStatsRemote(cServerIP, nPort, cAdminUser, cAdminPass, @nError)`
returns the stats hash, or NIL with a numeric error in the optional fifth
argument. Supply a literal IPv4 address or hostname (not a URI/path), port
1..65535, and credentials loaded from your secure settings. Empty credentials
retain only the existing literal-loopback read-only exception. This wrapper
builds host:port, uses its own native management handle, queries and disconnects,
serializes its lifecycle for MT callers, and leaves rddads/data handles alone.
The older `OAds_ServerStats(hMgmt, @nError)` remains available. Direct
AdsMgConnect also accepts tcp://host:port/; this is not tls:// support.

```harbour
LOCAL nError := 0, hStats
hStats := OAds_ServerStatsRemote( cServerIP, nPort, cAdminUser, cAdminPass, @nError )
IF hStats == NIL
   ? "Server stats failed", nError
   RETURN NIL
ENDIF
```

The explicit-wrapper smoke fixture is tests/smoke/harbour/serverstats_remote.prg.
Compile it with the actual contrib/oads_hb/oads_hb.c and matching ACE library
using hbmk2 -mt; run only against a sandbox daemon with test-only credentials.
It covers four concurrent callers, invalid port/URI rejection and wrong-login
failure. Passwords are never printed; production secrets do not belong in
command-line arguments or this fixture's invocation.

## Connection cap

max_sessions is the daemon's effective admission limit, not the connection
peak or open-file limit. 0 means unlimited. max_sessions_source records the
resolved command-line, openads.ini, environment or default source at startup.
Local-process/unsupported values are null, not fabricated caps. Both portals
show these fields and the Harbour hash includes them automatically. The
explicit-zero setting now overrides defaults as documented; omitted settings
still use environment/default. This does not change any deployment's settings.
