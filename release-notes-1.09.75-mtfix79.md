# OpenADS mtfix79 cumulative test kit

Source is cumulative78 plus an independent, default-OFF server table-open
phase diagnostic. All78memo-length and earlier fixes remain. This is a test
release, not a claim that Vouch or Tim's application is certified or faster.
No deployment, service registration, data/auth replacement or upstream merge.

## Server open timer

In the existing server INI, add one writable LOCAL destination:

    open_phase_log = C:/OpenADS-Server/logs/open6262.log

The parent directory must already exist. Use a different file per daemon
(e.g. open6262.log and open6264.log). On Linux use a full writable local path.
Restart that daemon to apply. Do not enable diagnostics=true or change client
logging to use this timer. Remove/comment the entry or set its value empty,
then restart to turn it OFF. Absent/empty is OFF. No environment switch or
client request can enable it. This explicit admin opt-in is independent of
the broad diagnostics master so it does not enable other logging channels.
Keep admin config and output directories private and permission-controlled.

Capture briefly. Synchronous local output adds overhead and can serialize
concurrent opens; do not keep it on for normal speed comparisons. No fsync or
durability promise for the diagnostic. At8MiB the sink stops appending and
disables itself for this process. No rotation/truncation. Stop capture before
moving/removing the file; restart after selecting an empty destination to
resume. An unavailable/unwritable destination disables only this timer;
no database error or request-result change is introduced.

Each OPEN79 line contains only fixed labels and numeric values:
- session: existing opaque numeric server session; request: process-local
  monotonic open sequence. Resets at restart; not an application/table ID.
- drivers: underlying CDX opens in this request, including any ABI twin.
- reply_opcode: numeric protocol reply opcode, not a success guarantee for
  socket delivery. Failure replies are recorded too.
- total_us: server process_frame capture, ending before diagnostic file write.
- engine_us: connection open_table call (resolution/registry/driver work).
- handle_us: CDX platform file open, including existing sharing retries.
- share_us: existing shared lifetime-guard acquisition/retry.
- header_us: CDX DBF header read and parse.
- fields_us: field-descriptor read/parse/install only.
- reply_build_us: handler work after engine open, including metadata, fresh
  production-index discovery and optional ABI/index setup.
- reply_submit_us: existing queue/write path and outgoing bookkeeping. With
  hardened daemon this is enqueue, NOT completed socket transmission or RTT.

All times are elapsed microseconds. CDX subphases aggregate underlying CDX
opens, may occur inside engine_us AND reply_build_us, and must NOT be added to
those parent times. Other dispatch/bookkeeping/optional encryption work can
remain unaccounted; this is not a complete additive time budget. Zero can
mean below1microsecond or a phase not reached. No paths, table/field names,
values, aliases, payloads, credentials, expressions or raw errors are written.
No wire extension, cache, existence/index-discovery shortcut, retry change,
lock/durability/RDD/GoCold boundary change or Vouch code change.

## Cumulative78 memo repair, also in matched MSVC79

AdsGetMemoLength settles outstanding remote read-ahead cursor lag before
fetching. No lag: one existing request. Lag: an extra positional request plus
the existing memo fetch. A memo-per-row scan can pay that extra request on
many rows. Not a free repair or WAN speed improvement. Failed settling returns
the error without fetching the wrong row or replacing the output length.
The generated forward/reverse regression checks39positions including empty
and200KB memos and stopped-server errors. This is not Tim's own application
runtime result. A server-only update does not repair an old client DLL.

## Test limits

Retain all auth, commit, navigation, diagnostic and mixed-version gates.
Windows x86 retains197known inherited failing case/assertion signatures;
new phase/config/integration cases increase coverage, not baseline failures.
Windows x64 and MSVC kits use native targeted checks, not a full native unit
suite claim. Linux/macOS retain configured full tests. macOS ARM/minimum-OS
runtime remains unverified. Build/verification reports give actual results.
Broader memo/count freshness, read-ahead/RDD parity, Tim C8and erratic browse
remain separate. Keep prior frozen kits and rollback files.

## Local overhead measurement (not Delhi/Mumbai)

GCC O1 NDEBUG release build, HTTP/SQLite/TLS OFF, generated one-field DBF on
warm local storage. Direct RemoteConnection open/close bypasses client parked
handle reuse.7randomized interleaved trials per condition,100warmups then
10,000real open/close pairs per trial,2CPU affinity. Median microseconds per
pair: frozen78 153.832;79OFF151.939;79ON151.737. Ranges overlap
(78:148.314..157.066; OFF:148.159..154.477; ON:140.854..154.792).
This noisy loopback test cannot resolve small end-to-end overhead; it is NOT
evidence that the timer makes opens faster or that logging is free. All70,000
ON opens emitted their expected rows. These timings do not describe Windows,
WAN, cold storage, antivirus, many-thread contention or the production load.

Isolated capture/sink helper (same O1 build,7trials x10,000captures, normal
local file/fflush, no database work): OFF median35.62ns/capture, ON median
2679.70ns (2.68microseconds). This measures fixed helper cost only, not the
server's full multi-thread cost; file-lock contention/slow storage can add
more. OFF is clock/file/lock-free in the helper; not a zero-instruction claim.
