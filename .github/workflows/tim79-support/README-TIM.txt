OpenADS cumulative79 MSVC test kit for Tim Stone
Pritpal Bedi, bedipritpal/OpenADS fork of FiveTechSoft/OpenADS.
Source: f7ad7d2d59f20b00f4eb6a6f9f2cc5ea7780140e (same as cumulative79).

Use x64 for the reported all64-bit setup. x86 is retained to match prior kit
packaging, not a recommendation to switch architecture. DLL, import library
and server in each archive are matched MSVC builds. HTTP and SQLite are ON;
TLS is OFF as in Tim's preceding64 MSVC configuration. Existing tcp:// testing
can continue. No settings/data/service change is performed by this archive.
Replace only during a planned test stop, preserve config/data and existing
rollback files. Test data only; no automatic deployment or data conversion.
Do not mix x86/x64, GCC/MinGW libraries or older DLLs with the new wrapper.

Contents
- openace32/64.dll and MSVC .lib import library; openads_serverd.exe;
  openads_auth_worker.exe if produced by the unchanged source build.
- include/ace.h and include/openads headers. Use ads.ch from your own Harbour.
- contrib/oads_hb/oads_hb.c (Harbour OADS_* wrapper source).
- patches/harbour/rddads-pending-fields.patch.
- docs/: cumulative75/78/79 contracts, diagnostic gate notes, build source pin,
  exact native test evidence and DLL export/dependency list.
- build-harbour-libs.cmd, for a matching existing Harbour source/toolchain.
No prebuilt Harbour VM or rddads library is substituted for your own ABI.

Harbour integration
The new marker cannot be added to an already-linked app by a DLL swap alone.
Compile contrib/oads_hb/oads_hb.c with YOUR Harbour headers and MSVC architecture,
then include that wrapper object in your rebuilt rddads.lib and link it plus
this kit's openace64.lib (or32) with your application.
Do not retain a second copy of OADS_* wrappers in a prior application library.
The DLL exports the C bridge OAdsTraceMarkEx; PRG name stays OAds_TraceMark.

The75 pending-field error bridge changes Harbour rddads/ads1.c, not the DLL
alone. Its patch is based on Harbour core4ec2e154ed92757418bc30af571ab90240ab3209.
Use a clean separate checkout of that exact commit or review/adapt the patch
against your own Harbour revision; a failed apply must stop for review. Rebuild
rddads with your own MSVC/Harbour ABI and the matching kit header/import library.
Do not use Pritpal's32-bit MinGW librddads.a. Do not apply an extra old rddads
compatibility patch blindly; duplicate/conflicting glue changes need review.

Provided build-harbour-libs.cmd builds the wrapper and four RDD objects with
cl/lib, after git apply --check and git apply of the pending-field patch to the
explicit Harbour source folder you pass. It changes that folder's ads1.c, so
use a disposable dedicated checkout. It refuses dirty source. Run in the matching
VS2022 native-tools prompt. Example (paths yours):
  build-harbour-libs.cmd C:\test\harbour-core 64
Use32 for an x86 kit. It writes only kit-local harbour-lib64/32 artifacts and
applies the stated patch to that dedicated Harbour checkout. It includes the OADS wrapper in rddads.lib, preserving the existing application
link model, not a separate wrapper-library migration. It does not replace your
installed Harbour libraries. Link explicit library paths so a stale rddads
elsewhere is not chosen first. Compile your app with compatible Harbour VM/MT
settings. This helper does not certify your entire app or your Harbour version.

Marker calls (application boundaries only)
  OAds_SetLogging( .T. )
  OAds_TraceMark( "PART_SAVE_BEGIN", nInstance, hb_threadId() )
  ...existing save/commit/unlock/restore calls, unchanged...
  OAds_TraceMark( "PART_SAVE_END", nInstance, hb_threadId() )
Set wire_trace=1, wire_trace_compact=1 and wire_trace_file to a writable LOCAL
path using the existing client config/environment before starting the process.
Both master logging and wire_trace must be enabled. Do not enable extra app
logging that adds database calls. Return .T./.F. is diagnostic success only,
not save success. Use FAILED/CANCELLED labels on those application paths.
Labels1..120 bytes, no control characters; no customer data/paths/secrets.
Optional integer IDs default0; exact integer0..9999999999999999, finite integral
floating values <=9007199254740991. Prefer actual integer counters. M rows are
local markers; only W rows count as wire frames. tick_us subtraction is within
one process/DLL lifetime only, not across instances. Local logging has overhead.

Speed retest
First repeat the original work-order/bulk benchmark with diagnostics OFF, same
schema/indexes/commit cadence/cache procedure/background workload. Record loaded
DLL/server versions and the RDD build. Then one short representative TEST save
with compact trace and BEGIN/END; trace runs are diagnostic, not untraced speed
results. Bulk add: keep original commit frequency, send a short representative
loop trace if needed. Do not remove commits, locks, checks or refreshes to make
a faster comparison. Source changes since64 reduce some request sequences, not
a demonstrated fix of the remaining SAP ADS gap.

Relevant cumulative changes
68 append reply metadata avoids certified follow-up record-number/lock trips.
71 qualifying multi-field batch + explicit commit can use one request instead
of SetFields then FlushTable.75 fresh-append first replacement joins that batch;
append ACK stays separate and existing-row first-field probing remains.
72 cold ordered-bottom scoped count can share a reply, sequence-dependent.
75 indexed-key edit + relative Skip re-anchor is a correctness repair, not a
C(8)-scope claim.77 local aligned instance/thread marker adds no wire request.
Both client/server needed for protocol capability wins and remote engine fixes.
Marker alone needs no server upgrade. New RDD bridge requires relink/rebuild.

Verification and limits
See packaged proof logs. This workflow checks both native MSVC architectures,
DLL/import API calls, local marker full/compact/gates/IDs, matched remote commit
and bottom capability cases, and TCP/HTTP startup. Exact wrapper sources and
headers ship for your Harbour ABI; native Harbour integration is checked only
if the corresponding proof log explicitly says so. No claim your MLS app was
run, the SAP gap is solved, C(8) scope is fixed, or this is production-ready.
Cumulative77 all-platform source gates passed separately. Existing MinGW x86
unit baseline has197 known failures; it is not a clean full-suite pass and is
not a native MSVC full-suite result. Original test release/tags stay frozen.

Cumulative78/79 additions
The memo-length logical-row repair is included. A lagged row needs one extra
positional request before the existing memo fetch; memo-per-row scans can pay
it on many rows. No WAN speedup claim. Native generated-data checks cover
39forward/reverse positions including empty and200KB memo contents; not your
application's runtime result.79also includes the default-OFF numeric server
open-phase timer; see docs/release-notes-1.09.75-mtfix79.md. It needs no Harbour
change and stays off unless the server admin sets open_phase_log in its INI.
