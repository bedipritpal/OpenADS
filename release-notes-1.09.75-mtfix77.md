# OpenADS mtfix77 full test kit: aligned instance and thread marker IDs

Requested and rule sheet reviewed by Pritpal Bedi. Built on his bedipritpal/OpenADS fork of FiveTechSoft/OpenADS, cumulative with the frozen mtfix76 kit. Test release only; no deployment, main merge or upstream submission is implied.

## Same Harbour function, optional application IDs

    OAds_TraceMark( "VOUCHER_SAVE_BEGIN", nInstId, hb_threadId() )
    // Existing Vouch save code stays unchanged.
    OAds_TraceMark( "VOUCHER_SAVE_END", nInstId, hb_threadId() )

The existing one-argument call remains valid. Omitted IDs default to0 (unspecified). Explicit0 is valid. An explicit NIL is not omission. IDs must be nonnegative whole numbers. Strings, logicals, arrays, negatives, fractions, NaN/infinity and out-of-range values return .F., without a Harbour error. No coercion and no silent truncation.

Exact Harbour integer values accept0..9999999999999999 (16 decimal digits). Floating numeric values accept0..9007199254740991 (2^53-1), finite and integral only. Larger floating values are rejected because their original integer identity may already have rounded. Prefer Vouch's actual integer counters and hb_threadId(), not IDs converted through floating calculations or decimal text parsing. Numeric construction before this call is application-owned; the function cannot recover information already lost by a caller.

The supplied thread ID is an application label, not a lookup or switch of an actual thread. Vouch supplies unique instance IDs and suitable thread IDs. The actual diagnostic thread identity remains separately available as auto_thread. Application IDs do not change connections or RDD execution.

## Aligned local marker rows in BOTH full and compact

Timestamp, blank wire-duration field, prior-row gap and existing trace columns remain. M rows have operation Marker and caller OAdsTraceMark. The detail columns, in order:

    tick_us.<20-wide>  auto_thread.<20-wide>  instance.<16-wide>  thread.<16-wide>  marker.<exact cMark>

Every numeric value is right-aligned in its fixed-width field. Instance, thread and marker start at the same positions on every marker row, including old one-argument calls with instance0/thread0. No decorative brackets. The marker is last and is not truncated. Full/compact use the exact same marker layout; compact still removes ordinary API summaries, not M rows.

Match application boundaries by supplied instance + thread + label, with auto_thread retained for execution provenance. tick_us is monotonic and relative to this client DLL/process's first marker, shared by both C entry points. Subtract ticks only within the same client lifetime, never across processes, DLL instances or restarts. It is application elapsed time with local logging overhead, not server processing time or a cross-thread/network barrier.

Only W rows are wire frames. Exclude M rows from frame counts and summed wire duration. M rows advance the previous-row gap column. Identify row kinds by columns, not by finding a keyword inside the application-supplied marker text. Use labels such as FAILED/CANCELLED on those application paths; marker success does not mean application save success.

## Unchanged diagnostic safety and deployment boundaries

The marker is client-local: no server connection, wire trip, table lookup, RDD call, pending-field flush, commit, lock/unlock, navigation, cache or database-last-error change. It respects BOTH existing wire_trace and master logging gates and never enables logging. Returns .T. only when local append and close succeed; .F. otherwise. A failed append can leave a partial row, so do not retry automatically or use its return as save status.

cMark remains1..120 bytes, no C0/DEL/C1 control bytes including embedded NUL/CR/LF/tab. Prefer short ASCII developer labels, never customer data, paths, credentials or secrets.

The published C OAdsTraceMark(label,bytes) ABI remains unchanged and now writes the aligned format with0/0 IDs. Additive C bridge OAdsTraceMarkEx(label,bytes,UNSIGNED64 instance,UNSIGNED64 app_thread) supports the same Harbour function. x86 MSVC wrapper/import checks retain the old @8 entry and add the @24 entry; MinGW keeps its existing cdecl convention. Do not change argument counts when calling the old C export.

The Harbour wrapper stays in the kit librddads.a as approved. Relink Vouch with this kit's matching wrapper, import library and DLL (or cumulative static client archive for static linking). A DLL-only swap cannot replace a Harbour wrapper already linked into an executable. New wrapper + older DLL is unsupported. The broader DLL-only OADS_* migration remains pending. No server upgrade is needed solely for marker IDs. Existing running installations and AV/security settings are untouched. Frozen mtfix76/52 and all other prior release assets/tags remain unchanged.

## Evidence and limits

Source gates cover omitted/zero/max/type/range validation, exact16-digit integers and floating precision boundaries, NaN/infinity, fixed column alignment in both flavors, shared old/new C marker clock/layout, multiple application threads and separate processes appending to one test log. The generated-data remote sequence with markers while fields are pending must remain the same26 wire operations/sizes. Actual linked Harbour tests verify numeric conversion and formatted values, not only return codes. Numeric fixtures use test-only C construction for floating boundaries because Harbour source-literal parsing can itself round a large decimal before the wrapper sees it; no test helper enters the production library.

Cumulative diagnostics, write/error, commit, navigation, auth and all-platform gates remain required. Windows x86 retains1821 executed cases and exactly the known197 failing names/assertion signatures, not a clean full-suite pass. Windows x64 retains binary/package checks, not a native full-unit claim. macOS ARM slice and macOS14 runtime are not independently certified. No universal Harbour/RDD parity, production readiness, real disk-failure recovery or Delhi speed diagnosis is claimed. Owner Vouch relink/runtime and a real Delhi capture remain unconfirmed.
