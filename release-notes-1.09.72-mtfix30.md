# v1.09.72-mtfix30 - server health in both management portals

Requested by and credited to Pritpal Bedi.
Fork bedipritpal/OpenADS, based on FiveTechSoft/OpenADS.
Test-branch pre-release only; no main or upstream merge.

Studio Server and DA-Web Server Info show the same schema1 statistics as
`OAds_ServerStats`, including Work Areas, current counts/sampled peaks,
traffic/operation totals, memory, distinct paths and Linux OS descriptors.
Unavailable/unmeasured remains explicit, not 0. Manual Refresh and raw JSON
are available; no new background polling or RDD behavior change.

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
