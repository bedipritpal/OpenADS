# v1.09.72-mtfix29 - OAds_ServerStats test build

Requested by and credited to Pritpal Bedi.
Fork: bedipritpal/OpenADS, based on FiveTechSoft/OpenADS.
This is a fork working-branch pre-release, not an upstream submission or
approval to merge into fork main. Keep main on its existing tested commit.

Adds OAds_ServerStats(hMgmt, @nError) returning a Harbour hash, using an
AdsMgConnect management handle. Aggregate health includes current counts,
existing sampled peaks, uptime, operations/errors, traffic totals, RSS,
version and Linux OS descriptor count/soft limit. Unmeasured rejected/parked
handle counts return NIL instead of false zeros. Workareas/table handles are
server-open handles, not OS descriptors or Harbour thread-local Select areas.

Existing ACE/AdsMg interfaces and wire snapshot remain unchanged. New export
and request are additive. Existing authentication applies to every remote
query, including the established loopback read-only exception. No record,
navigation, lock, RDD behavior or automatic background polling changes.

Use matching mtfix29 DLL/server and updated contrib/oads_hb/oads_hb.c.
Details and example: docs/server-health-stats.md. Test in the usual B_BIG
and Vouch environment before considering main. No live server changes made.
