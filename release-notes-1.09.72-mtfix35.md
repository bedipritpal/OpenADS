# OpenADS 1.09.72-mtfix35: physical LastRec parity and count I/O errors

Test pre-release for Pritpal Bedi's bedipritpal/OpenADS fork, based on FiveTechSoft/OpenADS. Pritpal's DBFCDX parity and integrity requirements guided this work. No private traces or business data are included.

## Changes

Ordinary DBF physical counts now use Harbour's `hb_dbfCalcRecCount` formula: `floor((file size - DBF header length) / record length)`, rather than `max(header count, size-derived count)`. A stale high header no longer creates phantom records or makes append allocate past the physical end. Deleted records are included. The optional trailing 0x1A EOF byte is discarded by integer division for ordinary record lengths, not explicitly subtracted.

Full-table encryption keeps fixed-length records and uses the physical-size formula. Partially encrypted tables have a bitmap after the record data and retain the header-count path, because total file size includes that trailer. This does not claim Harbour parity for OpenADS-specific encrypted formats.

Physical GetRecordCount refresh failures now return an error through the CDX/NTX, ADT and cache driver wrappers, Table, local ACE call and server count handler. A count output is not filled from stored state after refresh failure. Truncated DBF tests return error 5103 and leave the caller's output untouched. Other navigation refresh callers keep their existing error handling. Harbour's `hb_dbfRecCount` itself returns HB_SUCCESS without checking `hb_fileSize`; explicit error propagation here is the requested integrity safeguard, not a claim that Harbour has the same failure return.

Public remote table RecCount/LastRec still makes a fresh request per call. No sticky count cache, earlier-navigation reuse or wire protocol change. The mtfix34 unused internal request removal and earlier direct-rightmost CDX GoBottom fix are retained. No Vouch, Harbour RDD, main or upstream source changes.

## Verification

Fresh local Debug non-slow/non-flaky sweep: 1,640 cases / 613,935 assertions passed. Focused count, peer visibility, encryption and relative-position suite: 16 cases / 491 assertions passed. New cases cover high/low headers, empty/nonempty files, EOF present/absent, deleted rows, append allocation, zap, full encryption and a 64-row partial bitmap. Driver, local ACE and actual remote count errors are tested with a truncated file. The size-only regression cases fail against unchanged mtfix34 (19 failed assertions of 88), then pass with the fix.

Optimized normal/TLS guard passed on the clean rerun. An unchanged ZIP wrong-password fixture failed in the first run; it stayed enabled and passed the rerun. Full CI and all five packaging kits are checked before delivery. The existing slow/flaky surplus-key stress issue and exclusive-bag mismatch are not fixed here.

## Use

Update the server to get size-only remote count and server-side error propagation. Update the client DLL for local count behavior and the retained mtfix34 request removal. Windows x86 is the 32-bit Vouch kit. Standard Linux is for Ubuntu 24.04; Ubuntu 20.04 uses linux-glibc231. This is a correctness build, not a promised readiness speedup: a public remote count still costs one WAN trip.

No operation-scoped RDD snapshot API or file-size reply/header-length protocol extension is included. Those remain separate proposals for owner review.
