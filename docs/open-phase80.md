# mtfix80 timer-only cumulative test kit

Cumulative79 plus a finer numeric split of server OpenTable reply work. No
existence/index shortcut, cache, tag binding, lock, semaphore, integrity,
commit/durability, RDD, Vouch or wire behavior change. It measures the suspected
Windows open cost; it does not fix or prove its cause. Prior kits stay frozen.
No automatic deployment, service/config/auth/data replacement or upstream merge.

## Enable / disable

Keep the existing server INI and add under its server section:

    [server]
    open_phase_log = C:/OpenADS-Server/logs/open80-6262.log

Use a full private writable LOCAL path. Parent directory must exist and be
writable by the service account. Linux example: /openads-server/open80-6262.log.
Use a different file for each daemon/port, never a shared capture file. Restart
the intended existing service/daemon to apply. A manager stop/start may stop BOTH
6262/6264 and disconnect active clients; inspect its actual config routing first.
No diagnostics=true, environment switch or client request is needed or accepted.
Absent/empty means OFF. Remove/comment only this key or empty it, then restart.
Never replace the whole production INI with this example.

Capture briefly, then turn OFF. Diagnostic output is synchronous local append
and fflush, not fsync; concurrent opens may queue on the diagnostic sink.
The 8 MiB hard cap stops appends and disables the timer for this process, with
no rotation/truncation. Stop capture before moving/deleting its file; select an
empty destination and restart to resume. Unwritable/full/failed sink disables
only diagnostics, not database operations. Keep config and output private.

## Read OPEN80 rows

All values are unsigned numbers with fixed labels, no paths, names, expressions,
records, credentials or payloads. Microsecond times use a steady clock. Zero may
mean not reached or less than one microsecond. Numeric request is a process-wide
open serial, session is an opaque server session. Neither is a table filename.
reply_opcode is protocol status, not completed socket delivery.

Existing total_us, engine_us, reply_build_us, reply_submit_us, drivers and
handle/share/header/fields_us meanings remain from 79. drivers counts CDX table
opens, not index tags. Those driver timings are NESTED inside engine/reply.
reply_submit is enqueue/bookkeeping, not RTT. total ends before the log write.

New stage_*_us and stage_*_n count these coarse reply stages:
- book: registry/session bookkeeping and reply ID prefix, including failure path;
- discover: fresh production-bag search, case/path resolution and bag field;
- warm: schema and first-row/lookahead sections;
- twin: ensure_abi_handle, including any production bag auto-open/fallback;
- explicit: the outer explicit AdsOpenIndex refresh after twin creation;
- metadata: index IDs/tag metadata/path/expression packing and existing stats;
- reset: natural-order installation and twin positioning after index setup.
These stages occupy separate scopes in this path. reply_build minus their sum
is unclassified remainder, not automatically CPU or network time. Extra setup
section packing/dispatch and instrumentation overhead can be in that remainder.

Nested detail prefixes:
- other_: reply work outside the two named index passes; includes case resolution
  before the twin's production auto-open. Not the initial engine open.
- auto_: the twin's local production-bag auto-open inside stage_twin.
- refresh_: the subsequent explicit refresh inside stage_explicit.

Each detail has *_us and *_n:
- case: existing resolve_case_insensitive calls, including directory scans;
- dir_pass / dir_entry: COUNT ONLY, passes started / entries yielded by those
  existing scans; their *_us is intentionally 0. No extra scans or stat calls.
- tag_list: format sniff/listing work before choosing the tag-open branch;
- tag_open: attempted tag opens (including initialization/encoding/collation in
  that branch), not a kernel-only file-open duration. Failure attempts count.
- bag_wait: acquiring the existing per-bag create mutex (includes its lookup),
  not the subsequent existence check;
- bind_wait: acquiring the existing index-binding mutex;
- bind_work: snapshot/refresh/binding work while that mutex is held;
- heal: existing eligible-tag empty-root check, not the old full key count;
- reindex: actual existing reindex invocation, counted separately from checks.

Details are NESTED within coarse stages. Never add detail times to those stages,
reply_build or total. These counters are request-local, not shared index caches.
The existing index operations run in their original order with original locks.
Submicrosecond measurements and clock overhead limit tiny values. A large stage
identifies costly work, not its storage/OS/antivirus root cause.

## Validation and overhead

Generated raw-wire probe tests 15 OFF/ON opens with exact reply-byte equality:
multi-tag mixed-case .Z01, empty/no bag/missing/invalid bag, and read/shared/
exclusive modes. It verifies two tag opens in auto and two in refresh, separate
scan/heal counters and no extra phase row for standalone OpenIndex. A separate
memo probe checks 39 forward/reverse positions with exact contents/lengths,
including empty and 200 KB. These are fixture results, not Vouch/Tim certification.
Native build reports record which platforms actually passed these checks.

Local GCC O1 NDEBUG, HTTP/SQL/TLS OFF, warm generated two-tag .Z01 and 25 directory
neighbours; direct RemoteConnection bypasses ABI parked-handle reuse. Seven
randomized interleaved trials per condition, 100 warmups +3000 real open/close
pairs per trial, 2 CPU affinity. Median microseconds per pair:
  frozen79 OFF 508.222;80 OFF 515.236;80 ON 508.701.
Ranges:79 OFF 487.223..570.276;80 OFF 477.895..557.476;
80 ON 474.857..525.986. All 21000 ON opens emitted rows. Ranges overlap strongly;
this test cannot resolve small end-to-end overhead and does NOT show a speedup
or free logging. It is not Windows/WAN/cold/concurrent-production timing.

Isolated helper, seven trials x1000 synthetic captures with representative fine
stages, two index passes/two tags,25 entry counters, ordinary local fflush:
OFF median 129.667 ns/capture; ON 10741.1 ns/capture (10.7411 microseconds).
OFF bypasses Request construction and clock/file/lock work, but pays checks.
This is not a per-production-open guarantee; directory size, storage, scheduling
and sink contention can add overhead. OFF remains the default for speed tests.

## Delivery limits

Windows x86 combined client archive packaging is unchanged; its bundled server
is still x86. For the owner's 64-bit server standard, use the separate Windows
x64 server+worker pair. The proposed combined-archive architecture correction
was NOT included without separate approval. Client 32 remains supported.
No optional matched Tim MSVC80 kit is included. Frozen MSVC79 remains available.
Linux/macOS package structure remains unchanged; the existing macOS package has
no auth worker. macOS ARM/minimum-OS runtime remains unverified. Windows x86 keeps
its known 197 inherited failing assertion/case signatures; finer timer checks
must pass without broadening that baseline. x64 native probes do not constitute
a full native Windows x64 unit-suite pass. Full run results are reported after
CI finishes. Retain old binaries and a verified rollback route before deployment.
