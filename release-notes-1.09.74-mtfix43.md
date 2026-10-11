# v1.09.74-mtfix43: indexed-row wire padding

Requested and reviewed in design by Pritpal Bedi, with Harbour DBFCDX as the FIELDGET reference.

- Indexed RDD table rows omit trailing ASCII spaces only on fixed ADS_STRING values; the existing client restores declared width. Stored records and raw AdsGetRecord are unchanged.
- Embedded NULs, nontext bytes, full-width opaque values and genuine trailing spaces reconstruct byte-for-byte. No new opcode or client protocol version is needed.
- Natural-order engine packing and SQL cursor packing are unchanged; no new payload encryption is included.
- Native local regression: 150 records in reverse-of-RECNO index order, forward/backward lookahead refills, direct wire value checks and raw record equality. Full local suite: 1,727 passing cases, 163,716 passing assertions, 18 skipped.
- Synthetic mixed C(100) forward/backward scan: steady baseline 36,792 bytes, compact 18,895 bytes (48.6% less). Five-run median loopback time: 11.069 ms baseline vs 6.740 ms compact. Baseline first run was 33,749 bytes. These are fixture measurements, not a predicted Vouch/WAN improvement.
- All Windows static client kits use checksum-pinned Winlibs GCC10.2 (x86 posix-dwarf, x64 posix-SEH), compiler-runtime TLS/emutls enabled. Network TLS is independent and remains off for the lean static kit. No user compiler/Qt upgrade required.

Reference: Harbour src/rdd/dbf1.c hb_dbfGetValue returns fixed HB_FT_STRING at its declared width; DBFCDX inherits that implementation. Transport savings do not change this RDD contract.
