# OpenADS 1.09.74 - upstream-based integration candidate

DRAFT NOTES. Version name is proposed. This build has not been created.
STABLE RELEASE CANDIDATE. Not published or cleared yet.

The candidate starts from upstream main after the focused port queue, PR192,
commit 383dac64933db2e8eba0efaa0f96967f1ae4c99f. It keeps upstream's stronger
login, management authentication, malformed-frame checks and resource limits.
It does not restore older fork network behavior.

Thanks to Pritpal Bedi for the multi-instance Vouch tests, billing-counter
traces, stale-read reports and tea-break idle-session report behind these ports.

## Changes in the upstream-based candidate

- CDX bottom navigation reaches the rightmost index leaf directly and refreshes
  it for peer changes. Physical record counts and ordered-position counts use
  their correct contracts; ordered-position SET behavior is unchanged.
- Open replies carry bounded production-index metadata, with a matching-bag
  check before the client uses it. Server table paths reject symlink escapes.
- The mtfix39 lock-row refresh, mtfix40 remote-file wait handling and mtfix41
  healthy-idle session policy are retained in the upstream line.
- OAdsGetServerStats supplies bounded JSON health data for local or authenticated
  remote management handles. Harbour wrappers and a bounded text/GT display,
  Studio health GET and DA-Web health display consume it.
- DA-Web management credentials are kept in a server-side PHP session, scoped
  to the endpoint and expired after 20 minutes. Reconnect/disconnect, logout,
  expiry and endpoint changes clear them. Non-loopback credential entry needs
  HTTPS. All management JSON POST actions require the session CSRF token.

Existing clients that POST directly to DA-Web management endpoints must obtain
and send that CSRF token. The health display is read-only; management actions
still require their own authentication. Studio uses its existing auth gate,
no-store responses and manual refresh.

Management sockets remain plain TCP, including credentials. HTTPS between
the browser and PHP does not encrypt PHP-to-daemon management traffic. Use
a trusted management network or TLS proxy and protected PHP session storage.
DA-Web files are deployed separately from the binary archives.

## Compatibility and limits

Default remote record-count caching remains. The fork's opt-in
OPENADS_FRESH_COUNTS switch was parked during upstream review and is not in this
candidate. Do not treat a cached count as a peer-fresh count.

The daemon's established database-session idle cutoff remains disabled by
default. A positive established_session_idle_seconds server INI setting or
matching CLI flag opts into disconnecting inactive database sessions and
releasing their workareas and locks. Handshake, partial-frame, reply-drain and
management-session timers remain separate. Real disconnects still clean up.

Harbour remote health accepts a hostname or IPv4 address, not a URI or IPv6
literal. Its fixed 8192-byte buffer fails on a larger reply rather than showing
truncated JSON. App code must compile the matching OAds glue and use matching
DLL/import/static libraries. TLS and local-process health are separate from
whether a particular application reconnects safely after a real failure.

No automatic reconnect or replay is added. This does not guarantee survival of
host OOM/SIGKILL or unlimited client sessions. No fresh Vouch application test
of this candidate has been claimed.

## Validation before publication

The reviewed upstream head with the same code tree passed all 11 source CI
checks: Windows x86/x64, macOS, Linux normal/TLS, Harbour, PHP and SQL gates.
https://github.com/FiveTechSoft/OpenADS/actions/runs/38024837993

Fresh local Linux normal builds of ACE and the daemon report 1.09.74.
A selected 50-case/1,027-assertion health, path-jail, open-budget, management,
idle-policy, CDX and encryption regression run passes. The PHP management
sandbox and renderer contracts also pass. These are selected local checks,
not full CTest or new cross-platform release validation.

That source CI is not validation of the new packaging patch. The candidate's
five package builds, MinGW static/import probes, archive integrity/version
checks and application test clearance remain pending. These notes must be
updated with the actual candidate build result before publication.

## Planned packages

Windows x86 and x64 ZIPs carry both ACE DLL names, MSVC imports, refreshed
MinGW DLL imports and MinGW static ACE libraries. Linux x64 and glibc2.31
archives, plus macOS universal, carry both shared-library names. All carry
matching headers, OAds glue and stats display source, license, notices and
sample server INI. The glibc2.31 package is for older Linux systems that cannot
load the Ubuntu24-built library. Check the actual package before choosing it.

## Application check

The stable release remains blocked on Pritpal's review of the exact content and
his Vouch clearance. Use an isolated installation for that clearance. Match DLL/import libraries/glue to this build.
Test multi-instance billing paths, record-lock exclusion and refresh after lock,
peer append/bottom navigation, module open/close, healthy idle longer than five
minutes, real disconnect cleanup and login-lock reacquisition. Exercise local
and authenticated remote health, HTTPS management entry, logout/expiry and
legacy actions with CSRF. Confirm the intended count-cache behavior.
