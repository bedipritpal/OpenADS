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
