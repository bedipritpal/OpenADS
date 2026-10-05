# Server health statistics (mtfix29 test)

Requested by Pritpal Bedi, on bedipritpal/OpenADS fork of FiveTechSoft/OpenADS.
No upstream submission or automatic main merge.

Compile/link updated contrib/oads_hb/oads_hb.c and use mtfix29 DLL and server.
No RDD changes: DBFCDX apps can call it if linked to OpenADS.

    LOCAL hMgmt := 0, nError := 0, hStats
    IF AdsMgConnect( "127.0.0.1:6262", "", "" ) == 0
       hMgmt := AdsMgGetHandle()
       hStats := OAds_ServerStats( hMgmt, @nError )
       IF hStats != NIL
          ? hStats[ "version" ], hStats[ "uptime_seconds" ]
          ? hStats[ "workareas" ][ "current" ]
          ? hStats[ "os_fd_count" ], hStats[ "os_fd_soft_limit" ]
       ELSE
          ? "Health query failed", nError
       ENDIF
       AdsMgDisconnect()
    ENDIF

Empty credentials above use ONLY the existing loopback read-only exception
when no management credentials are configured.
Remote management needs configured management credentials. Do not put passwords
in logs/source. hMgmt MUST come from AdsMgConnect, not AdsConnect60 or the
OAds default connection. NIL means failure; optional @nError receives ACE code.
This makes no background poll, RDD/navigation/lock mutation or credentials bypass.
Old AdsMg structs and wire snapshot are unchanged. Old servers fail the new
request instead of substituting local statistics.

Schema1 fields:
- version, scope (server/local_process), server_port
- users, connections, workareas, table_handles, index_bindings, locks:
  current/max_used/rejected hashes
- worker_threads.current; max_used/rejected NIL because unmeasured
- distinct_table_paths, distinct_index_paths: unique exact path strings,
  not canonical inode identities or physical OS descriptors
- uptime_seconds, operations, logged_errors, rss_bytes
- packets_in/out, bytes_in/out, disconnects, partial_connects
- os_fd_count/os_fd_soft_limit: reporting process's Linux count/soft limit;
  NIL if unavailable (unlimited soft limit is also NIL)
- parked_handles: NIL (client-only state)
- semantics: strings documenting counting limitations

Workareas/table_handles both retain existing server-open handle counts including
client-parked handles, NOT Harbour Select() areas or OS descriptors. Distinct
paths are separate. Users are management entries, not unique people/Vouch
instances. Multiple sessions/lanes can belong to one application. Index bindings
can include several tags/orders. Rejected is NIL throughout, not the hardcoded
AdsMg0. max_used retains sampled high-water marks, not guaranteed instantaneous
peaks. Snapshot is best-effort concurrent; query's own management session can
be included. Comm totals can reset through existing management operations.
RSS0 retains the existing helper's unavailable sentinel.

Cost scales with sessions/tables/indexes/locks and Linux descriptors. Reads
metadata only, not file data or records; call on demand or at a modest interval,
not every DB operation. Output has no private paths, usernames, SQL or credentials.
Versioned JSON avoids pointer/packing dependence between x86 client/x64 server.
Use Harbour numeric values without coercing to 32-bit C ints; totals above2^53
may lose precision in configurations using double numerics.

C: OAdsGetServerStats(hMgmt, buffer, &length). 32-bit length is capacity INCLUDING
NUL, replaced by required size INCLUDING NUL. NULL/short buffers give
AE_INSUFFICIENT_BUFFER without partial writes. Allow8192 bytes for schema1.
Use schema_version checks for future consumers.
