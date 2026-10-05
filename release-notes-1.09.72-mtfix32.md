# v1.09.72-mtfix32 - fewer WAN open-setup calls

For Pritpal Bedi's bedipritpal/OpenADS fork, based on FiveTechSoft/OpenADS.
Pritpal's Vouch readiness diagnostics guided this work. Private traces and
application data are not included. Test-branch pre-release only.

Production index binding/tag metadata and immutable record length now ride
in OpenTableAck. The client uses those bytes instead of separate OpenIndex
and GetRecordLength wire requests. Record counts, FileExists checks, lock
rules, durability and per-session handles/cursors are unchanged. Nothing
changes in Vouch or Harbour DBFCDX.

New client capability gates index binding on the new server. Old clients
keep the existing path; new clients fall back on old servers. Natural order
is retained at open. Existing CloseAll/index park and same-bag dedup paths
remain in use. Record length comes from the opened driver's header, not an
estimate or a field-width sum.

Sandbox checks cover no extra index/length frames, old-client and old-server
fallback, two-login cursor independence, same-bag reopen, warm row state,
index park and teardown behavior. Guarded optimized normal/TLS builds passed.
The existing local exclusive-bag 6106-versus-7040 fixture mismatch reproduces
in the prior unmodified sandbox too; this release does not claim to fix it.

No readiness speed claim yet. Deploy matching client and server kits for
the new path, then measure the usual readiness boundary. With an older
server the client keeps the legacy calls. Main/upstream and live servers
are untouched; fork main still needs Pritpal's application test clearance.
