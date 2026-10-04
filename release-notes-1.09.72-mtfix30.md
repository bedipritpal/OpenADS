# v1.09.72-mtfix30 - server health in both management portals

Requested by and credited to Pritpal Bedi.
Fork bedipritpal/OpenADS, based on FiveTechSoft/OpenADS.
Test-branch pre-release only; no main or upstream merge.

Studio Server and DA-Web Server Info show the same schema1 statistics as
`OAds_ServerStats`, including Work Areas, current counts/retained workarea and table-handle peaks,
traffic/operation totals, memory, distinct paths and Linux OS descriptors.
Unavailable/unmeasured remains explicit, not 0. Manual Refresh and raw JSON
are available; no new background polling or data RDD behavior change.

Studio uses its existing HTTP auth. DA-Web adds separate endpoint-scoped
management credentials held in the PHP session for 20 minutes, with Forget,
expiry and DD-disconnect cleanup. No DD credential reuse or password in URLs,
localStorage, logs or returned JSON. HTTPS credential entry and CSRF-gated
login/logout. Existing daemon management authentication remains unchanged.
Management wire passwords still require a trusted/firewalled route/TLS proxy.

Matching DLL/server kits include the Studio changes. DA-Web is a separate
PHP deployment: use the DA-Web files from feat/portal-health-mtfix30.
Details: docs/portal-server-health.md. Usual B_BIG/Vouch application clearance
is still needed before fork main merge. No live server changes were made.

MinGW link repair: fresh ace32/ace64 import libraries are generated from
the built DLL exports, including OAdsGetServerStats. The mtfix29 MinGW
import archive was stale despite the correct DLL; both32-bit calling
conventions are link-probed before packaging. No application PRG change.

Harbour convenience: `OAds_ServerStatsRemote(cServerIP, nPort, cAdminUser,
cAdminPass, @nError)` builds the endpoint and owns a serialized management
connect/query/disconnect lifecycle. Hash on success, NIL plus numeric error
on failure. It leaves existing rddads management and data handles unchanged.
Supply credentials from secure settings, not hardcoded examples. Existing
`OAds_ServerStats(hMgmt, @nError)` remains. Direct AdsMgConnect now accepts
tcp://host:port/ too; this does not add native tls:// management support.

Workarea/table-handle peaks now retain transient opens between snapshots.
Other max fields keep their existing telemetry semantics; current counts
remain concurrent samples. These are logical handles, not OS descriptors.
