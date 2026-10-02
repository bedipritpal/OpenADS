Built for Pritpal Bedi - bedipritpal/OpenADS, forked from FiveTechSoft/OpenADS.

# v1.09.72-mtfix25 - wide CHARACTER field test build

Test build only, not a production recommendation. Use matching client/server kits and back up binaries and data before testing. Fork main is unchanged; Pritpal Bedi's Vouch test remains its merge gate.

## The bug

A DBF field descriptor stores a CHARACTER width in two bytes. OpenADS read only the first byte and treated the second as decimals. Any character field wider than 255 was therefore clipped: C(266) opened as C(10), C(300) as C(44), C(1024) as C(0), and every field after it shifted to the wrong offset. Values were cut at the wrong width on save and read. Native Harbour DBFCDX computes width = byte16 + byte17 * 256 with no decimals (src/rdd/dbf1.c, hb_dbfOpen).

## What changed

- Reading a table now takes the full CHARACTER width and cascades field offsets correctly. LOGICAL is always 1 byte and DATE is 8 bytes (3 or 4 for binary dates), as Harbour does. Other types keep their width and decimals.
- Creating, altering, copying and exporting tables now write the high byte for CHARACTER fields and the matching field displacements. AdsCreateTable accepts CHARACTER widths up to 65535; before, widths above 255 were ignored.
- The server no longer drops values longer than 4096 bytes. It sizes each non-memo cell from the field width. SQL group-by, projection and DDL structures carry 16-bit widths.
- New regression test dbf_wide_char_test: descriptor bytes, offsets, create, save and read-back of C(266), C(300) and C(1024).

## What to test

Re-save the browser configs once so they are written with the corrected layout, then check that they persist. Tables already damaged by an older build (written through the clipped layout) are not repaired.

## Limits

A CHARACTER field is at most 65535 bytes. Remote (server) mode with wide fields has not been exercised here, and the SQL group-by/temp-row path above 64K is unmeasured. The earlier macOS and ADT merge-gate notes from mtfix24 still apply.
