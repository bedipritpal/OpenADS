# OpenADS mtfix76 full test kit: local application trace markers

Requested and boundary rules reviewed by Pritpal Bedi. Built on his bedipritpal/OpenADS fork of FiveTechSoft/OpenADS, cumulative with the shipped mtfix75 kit. Test release only; no deployment, main merge or upstream submission is implied.

## Application API

    OAds_TraceMark( "VOUCHER_SAVE_BEGIN" )
    // Existing Vouch save code stays unchanged.
    OAds_TraceMark( "VOUCHER_SAVE_END" )

Returns .T. only if the local marker row was appended and the file closed successfully. Returns .F. when diagnostics are disabled, the argument is invalid, or the local append fails. Do not use the result to decide whether a save succeeded. A failed append can leave a partial row; do not retry automatically. Use FAILED/CANCELLED labels on those application paths. The application chooses what each boundary encloses, including any restore work.

Markers appear in BOTH full and compact cli_trace.log. Same configured destination, timestamp and plain columns; operation Marker, flag M, blank request-duration column. Details have thread ID, steady-clock microsecond tick and the exact label. The steady origin is initialized at the first marker in this client instance. Subtract matching BEGIN/END tick_us values on the same thread to measure the enclosed application interval. Local logging itself has overhead: zero network trips does not mean zero measurement overhead. Concurrent threads can interleave. These are local observations, not network barriers or server timing.

Only W rows represent completed wire frames. Exclude M rows from wire frame counts and summed dur_us. Marker rows advance the existing previous-row gap column. Existing analysis must account for that rather than treating every row as a request. Labels are application-supplied text, so classify by the row flag, not by finding a keyword inside label text.

The function requires BOTH the existing wire_trace opt-in and the OAds_SetLogging master gate. It never enables logging. No server/connection is required. No wire frame, table lookup, RDD call, pending-field flush, commit, lock/unlock, navigation, cache or last database error change. No protocol or server behavior change; no server upgrade is needed solely for markers.

Labels must be strings of 1..120 bytes, no C0/DEL/C1 control bytes, including embedded NUL, CR, LF or tab. Empty/oversized/invalid input is rejected, not silently truncated. Prefer short ASCII developer labels. Labels are written verbatim: never put customer data, paths, credentials or secrets in them. This is NOT a database-data logging API.

C API: OAdsTraceMark(const UNSIGNED8* label, UNSIGNED32 bytes), length-bearing so embedded NUL is detectable. AE_SUCCESS means written, nonzero means not confirmed written. It deliberately does not replace ACE's last database error. Header and DLL exports are additive. x86 MSVC has cdecl and stdcall entry points; MinGW has its existing cdecl form. Matching build-generated import libraries must include OAdsTraceMark.

The Harbour wrapper stays in today's kit librddads.a, as Pritpal approved for this addition. Relink Vouch using the matching kit wrapper/import library/DLL (or cumulative static archive for static clients). A DLL swap alone cannot add the Harbour symbol to an existing executable. Do not mix the new wrapper with an old DLL. The larger DLL-only OADS_* wrapper migration remains pending and is not part of this kit.

## Checks and limits

The source includes fresh-process environment/INI and full/compact marker gates, invalid/binary input and 120-byte boundaries, unwritable destination, concurrent marker rows, stable thread/tick ordering and unchanged last database error. A generated-data remote probe compares the exact 26-frame operation/size sequence with and without four markers, including boundaries while field assignments are pending. Actual linked Harbour checks cover argument types, embedded NUL/control rejection, full/compact and disabled gates without creating a connection.

Cumulative diagnostics, append/error, commit, navigation, auth and platform release gates remain required. Windows x86 retains the known 197 failing unit cases and exact assertion signatures, not a clean full-suite pass. Windows x64 retains binary/package checks, not a native full-unit claim. macOS ARM slice and macOS14 runtime are not independently certified. No universal Harbour/RDD parity, production-readiness, disk-failure recovery or Delhi speed diagnosis is claimed. Current Delhi save timing still needs a real application capture.

Frozen published releases/tags, including mtfix52, remain unchanged. No running server, security policy, AV exclusion or database was changed.
