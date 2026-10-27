# Application-controlled diagnostics

The master is OFF by default. No environment or INI entry can enable the
client master. Existing Harbour binding: `OAds_SetLogging(lOn) -> lOk`.
C API: `OAdsSetLogging(UNSIGNED16 usOn) -> ACE status`.

The application must explicitly call the existing API with true in the
same process to permit diagnostic output. Calling false blocks diagnostic
output again, including an already-installed wire hook. A separate helper
process cannot authorize an already-running client. No Vouch behavior or
startup-option change is implied by this API.

Once the application permits diagnostics, the existing env/INI selectors
choose channels as before. Client wire selector `OPENADS_WIRE_TRACE=1`
(or `wire_trace=1`), destination `OPENADS_WIRE_TRACE_FILE` (or
`wire_trace_file`). Default destination remains `C:/tmp/cli_trace.log`;
its parent directory must exist. Selection and file destination are cached,
so restart after changing them. The application master itself is dynamic.

The standalone server uses separate explicit admin-owned server config:
`diagnostics=true`, default false. The environment does not set this master.
Config-file/service permissions must restrict who can edit it. Restart to
apply server config. Client tracing does not require server tracing.

Trace lines retain operation, byte counts, durations and safe table IDs.
Real paths/names, filename-derived aliases, index expressions, SQL and raw
error text are not persisted. Distinct explicit aliases remain visible.
Auth proof frames and request/reply payloads are excluded. API errors still
return their normal text to the application. Database recovery journals
are data, not diagnostics, and are unchanged.

The file is appended, not cleared or rotated by the master. Existing raw
files from older versions are not sanitized by an update. Capture briefly,
turn the application master off, and review files before sharing. Trace
writes add overhead; traced wall-clock time is not a clean speed benchmark.
