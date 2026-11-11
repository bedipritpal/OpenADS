# OpenADS mtfix78 full cumulative test kit

Prepared for Pritpal Bedi, on his bedipritpal/OpenADS fork of FiveTechSoft/OpenADS. Cumulative with frozen mtfix77. Test release, not production clearance. All prior releases/tags/assets remain unchanged.

## Memo length follows the logical row

Pritpal delegated this memo repair after reviewing the rule and extra-request cost. AdsGetMemoLength now settles outstanding remote read-ahead cursor lag before fetching the memo. A failed settle returns the error without fetching from a different row or changing the caller's length output. No outstanding lag means no added positional request.

The repair does not remove read-ahead, add a memo cache, change locks, semaphore checks, hot-record/commit boundaries, file formats or Vouch source. It uses the existing positional Skip request. This is a client DLL repair; a server-only upgrade does not repair an old client. Existing server75 can serve it. Use this kit's matching DLL/import/wrapper and daemon for the intended controlled comparison. No forced deployment or auth/config replacement.

## Measured correctness and cost

Frozen77 synthetic loopback baseline returned the wrong memo length at10 of12 checked positions. The private candidate returned all12 correctly. Expanded forward/reverse checks verify lengths and contents at39 positions, including empty and200,000-byte memos. The retained regression also checks a stopped-server error leaves output untouched. These are generated DBF/FPT data, not Tim's application or Vouch runtime certification.

Measured requests in the expanded synthetic test: no-lag length read1; lagged length read2 (settle plus existing memo fetch); immediately following GetString0. At most positions in a memo-per-row scan, local Skip recreates lag, so this repair adds a request and can reduce read-ahead's WAN speed benefit. Not a free repair or speed improvement. No physical-disk timing or Delhi/Mumbai speedup is claimed. Broader memo freshness, indexed/filter/relation matrices and all RDD parity remain separate.

## Retained line and limits

All cumulative77 features remain:71 commit fusion,72 GoBottom count metadata,73 caller diagnostics,75 indexed-key Skip/error repair,76/77 local markers and application IDs, prior Tim fixes, refresh and auth layers. Existing diagnostics remain opt-in and default OFF. The proposed server table-open phase timer is NOT included. No new round-trip fusion, retry policy, AV policy, browser rewrite, count-cache policy or general failed-write repair is included.

Windows x86 must retain the same197 inherited failing case/assertion signatures and zero harness errors; the added memo test raises executed cases from1821 to1822. It is not a clean full-suite claim. Linux/macOS retain configured full tests; x64 Windows retains package/ABI checks rather than a native full-unit claim. macOS ARM and minimum-OS runtime remain unverified. Auth/commit/navigation/diagnostic/mixed-binary gates remain required. Final platform results are recorded by the build and verification report, not assumed from local tests.

The broader count freshness, read-ahead/RDD parity, Tim C8, erratic browse and inherited test backlogs remain open. No main/upstream merge or running server change is implied by this test release.
