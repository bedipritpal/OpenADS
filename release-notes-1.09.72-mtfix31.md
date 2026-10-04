# v1.09.72-mtfix31 - coordinate-aware console stats

Requested by and credited to Pritpal Bedi, following his B_BIG display
feedback and Vouch wiring plan. Fork bedipritpal/OpenADS, based on
FiveTechSoft/OpenADS. Test-branch pre-release only; no main/upstream merge.

Adds `OAds_ServerStatsShow(hStats,nTop,nLeft,nBottom,nRight)` to the existing
`contrib/oads_hb/oads_stats_display.prg`. It uses DispOutAt in the current
console/Clipper Tools window coordinates and current color, clears only the
inclusive given rectangle, clips long lines and excess rows, returns drawn
line count, and preserves cursor/color. No border is drawn: pass your box's
inner rectangle. Use76columns for the complete current/max/rejected layout.
Omitted coordinates use currentrow/column through MaxRow/MaxCol. Invalid
coordinates return0 without drawing; bottom/right beyond the window clamp.
`OAds_ServerStatsText` remains unchanged for text/log output.

Native Harbour sandbox tests inspect actual GT screen cells, outside-area
preservation, colors, cursor, clipping, NIL/invalid inputs and a nonzero-origin
Clipper Tools window. Screen-cell output was rendered and visually checked.
Example: `nRows := OAds_ServerStatsShow(hStats,nTop+1,nLeft+1,nBottom-1,nRight-1)`.
Compile the kit PRG alongside your app and the kit's oads_hb.c.

Includes mtfix30 portal health, separate DA-Web management login, remote
wrapper, retained workarea/table peaks, configured cap/source and fresh MinGW
imports. No server stats/RDD semantic changes in31. DA-Web still deploys
separately from the test branch. No live server changes were made; fork main
still waits for Pritpal's application test clearance. His EC2/Harbour feedback
is credited; it is not a blanket production test claim.
