# OpenADS mtfix75 cumulative test candidate

Includes the paused mtfix74 indexed-key Skip repair and fixture cleanup, plus append-record replacement batching and a matched Harbour RDD error bridge. Timing work is excluded. No live installation is changed.

## Replacement boundary

APPEND BLANK remains its own acknowledged operation. On a successfully appended current record with valid generation and locks, the first replacement joins later replacements in one batch. Each explicit dbCommit remains a distinct boundary, including a key-only commit followed by a data commit. Movement, another append, unlock and close drain pending replacements before their next operation. Movement does not add a durable commit or release application locks. Clean commits and clean pending-field drains do not add a request. Existing-row first-field probing remains unchanged.

Own pending field reads use the buffered value before fetching a row. An old append acknowledgement without the required metadata keeps the prior first-field probe behavior. No new wire opcode is added. Server errors from missing columns and twin flushes are checked and returned; partial applied prefixes are not rolled back or automatically retried. A lost acknowledgement invalidates the connection rather than replaying an uncertain write.

## Matched library requirement

The kit Harbour library must be rebuilt with patches/harbour/rddads-pending-fields.patch against Harbour4ec2e154ed92757418bc30af571ab90240ab3209. Relink Vouch with the updated librddads.a. A DLL swap alone does not add RDD error checks. New RDD code imports AdsFlushPendingFields; use the matching new DLL and import library. New RDD plus an old DLL is unsupported and must fail for the missing entry, not fall back to a forced commit. No Vouch source change is required.

The additive AdsFlushPendingFields(table) entry drains pending field assignments only: no commit, movement or unlock, and no request when clean. The RDD calls it before Goto's historical invalid-record compatibility path and before a new append, so prior write errors are not mistaken for navigation or ordinary append failures. Ordinary append errors retain the stock Harbour category/subcode/default flags. APPEND BLANK expands to dbAppend(); the command ignores the boolean. The default append-lock handler still sets NetErr() and allows dbAppend() to return false. Other errors follow the existing handler.

## Evidence and limits

Local selected C++ and actual Harbour runtime tests pass. Harbour tests exercise successful batches, error boundaries, default append-lock behavior, ordinary append errors against stock baseline, deferred write failure before append, generic Goto5025 compatibility, refresh tails and SQL close callers. SQL close tests inject SQL-success/no-cursor before the actual RDD close path; they do not certify engine SQL execution with a simultaneous close fault. Flush-helper fault tests are synthetic, not a real disk-failure test. Windows and all-platform candidate gates must pass before publication. No universal any-error, all-platform parity, rollback, safe-retry, Vouch-runtime or production-readiness claim.

The x86 full default test gate retains the existing197 failing cases and exact assertion signatures, with no new failures or harness errors. It is not a clean full-suite pass. Linux and macOS retain their configured full ctest gates; Windows x64 retains binary/package checks rather than a native full unit-suite claim. All prior auth, refresh, commit, bottom, diagnostics and mixed-server gates remain. Existing73 releases are frozen. Remote key-Skip repair requires the new server; a new DLL cannot repair an old server's hidden cursor.

## Retained indexed-key Skip repair details
# OpenADS mtfix74 cumulative indexed-key Skip repair
Pritpal Bedi. 8 November 2026.

Frozen73 plus a bounded engine repair for indexed-key edit followed by relative Skip. Successful GoCold/index maintenance marks a lazy per-index re-anchor; Skip re-finds the current physical record under its new key before walking forward/backward. Parked tags retain their own pending state; replacement index objects do not inherit it. Duplicate runs have no new arbitrary cutoff. Failed re-anchor invalidates the cursor and returns the error. Server Skip now checks cursor, ordered-twin and natural-table failures instead of issuing a normal ACK.

AAA to AAZ then Skip(1) must land on BBB, following Harbour DBFCDX GoCold/current-key refresh ordering. Local repair requires the74 DLL; remote repair requires the74 server. Old73 client/new74 server is included in the compatibility gate. New74 client/old72 or73 server retains the old server defect; a client DLL cannot repair that hidden engine cursor. No wire-format change, automatic unlock or forced hard flush. User locks remain held; later explicit commit still handles pending durability. No Vouch/new C edit, count-caller, read-ahead or OPEN-policy repair. Frozen releases remain unchanged.

No extra re-anchor seek is added to indexed append/commit itself. The cost is deferred to relative navigation after relevant key maintenance. Tests check hot and explicit-commit edits, forward/backward, key crossing, duplicate runs above1024, Skip0, scopes, local descending, multiple tags, conditional state lifetime, disk bytes, retained locks and injected failures. Remote descending API remains unsupported and is not certified. Conditional membership navigation is not broadly redesigned; lifecycle bookkeeping and physical Goto outside a FOR tag are checked. Existing partial data/index maintenance on failure is not rolled back; an error does not certify a safe retry. GoTop/Seek masked dirty failures and other handlers remain outside this repair. This is not full Harbour parity or production-readiness certification.

Retained71 commit fusion,72 GoBottom metadata and73 caller diagnostics remain cumulative. Diagnostics default OFF. The x86 gate requires1814 executed cases, exactly the existing197 baseline failing names/assertion signatures, all12 new indexed-key Skip cases passing, and zero harness errors. Linux/macOS retain their full configured ctest gates. Windows x64 retains PE/package checks, not a native unit-suite claim. New12-case local matrix passes7707 assertions; combined targeted local runner passes106 cases/25841 assertions. These local checks do not replace all-platform release gates.

## Retained73 diagnostic detail and72 limits
Frozen72 plus one fixed-width ACE trigger/reason column in completed client wire rows. Scoped public-entry context, not sticky last-call logging. Examples: AdsWriteRecord/batch+commit, AdsUnlockTable/buffered-sets, AdsGetRecordCount/request, AdsGotoBottom/internal-keycount. The 48-character column follows the W/blank flag; prior leading columns remain in place. Full/compact retain identical operation/size sequences. Blank-flag API summaries show a dash.

Outer ACE API is the trigger. Request is a neutral default dispatch reason, not proof of a direct application call. Batched writes identify the dispatch trigger, NOT the origin of each buffered assignment. Call/thread IDs are process-local log correlation; current nested API is in detail. Nested calls restore context on return/exception. Unknown/uninstrumented or unpropagated cross-thread dispatch is printed unknown with zero IDs, never attributed to a stale previous call. No literal Vouch PRG function/line, RDD callsite, native stack or app-versus-RDD attribution is claimed.

Static API/reason labels and numeric correlation only: no stack walking, new addresses, paths, field arguments, payloads or auth data. Existing diagnostics master remains OFF by default, requires application opt-in, and channel selectors cannot turn the master on. Complete-pair/frame coverage limits and existing masking remain. All 663 entries in ace_exports.cpp are scoped; other entry surfaces are not certified. No zero-overhead or application speed claim. Disabled scopes skip TLS updates/IDs but master/selector checks still cost something; enabled diagnostics add context and longer file rows.


See release-notes-1.09.75-mtfix73.md for the retained caller-column semantics and72 count limitations.
