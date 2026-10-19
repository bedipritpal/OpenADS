# Mutual auth (auth:// V1 + V2) - merged record, fork main

Auth design and end-to-end testing by Pritpal Bedi on the bedipritpal/OpenADS fork of FiveTechSoft/OpenADS.

Status 2026-10-19: fully merged in fork main (bedipritpal/OpenADS). Fork only,
nothing upstream. Merge commits: PR #14 (2026-10-16 20:46 UTC, mtfix49/50/51
bundle) and PR #15 (2026-10-17 13:53 UTC, mtfix52 V2 test workflow). The
test/mtfix52-linux-server-kit branch differs from main only by the kit-build
workflow; the product code is identical. No code merge is pending.

## Milestones

- M1  2026-10-15 03:24 UTC  mtfix47 mutual-auth spike branch: worker + channel binding explored.
- M2  2026-10-15 10:14 UTC  mtfix48 spike: mutual proof flow refined.
- M3  2026-10-15 14:11 UTC  mtfix49 auth-main branch: production-shaped worker and session integration.
- M4  2026-10-16 20:17 UTC  mtfix50 diagnostics: BeginAuth error propagation to the client.
- M5  2026-10-16 19:33 CDT  Owner localhost Vouch clearance on mtfix51: duplicate user, multi-instance, multi-thread, indexes, all through auth:// ("You can merge in main.").
- M6  2026-10-17 00:38 UTC  Bundle commit 64418d4: mtfix49 auth + mtfix50 errors + mtfix51 legacy root restoration, merge-ready.
- M7  2026-10-16 20:46 UTC  PR #14 merged to main.
- M8  2026-10-17 13:53 UTC  PR #15 merged: mtfix52 mutual auth V2 test workflow (Windows kit).
- M9  2026-10-17 18:15 UTC  test/mtfix52-linux-server-kit branch: Linux kit workflow only.
- M10 2026-10-18 10:27 UTC  Design note docs/vouch-as-a-service.md on main.
- M11 2026-10-19 00:32 CDT  Owner loopback V2 login passes, dummy key both halves (127.0.0.1:6262).
- M12 2026-10-19 01:06 CDT  Owner loopback V2 login passes, real key both halves. Production grade.
- M13 2026-10-19 01:50 CDT  Owner WAN V2 login passes with his own .hrb on EC2 us-west-1 (auth://13.56.76.69:6262/C:/). V2 proven end to end over WAN.

## What shipped, big items

1. Auth worker process model (tools/serverd/auth_worker_main.cpp, new, 624 lines).
   serverd spawns supervised worker processes; bounded stdin/stdout IPC;
   2 s per-call deadline; fixed concurrency slots with no queue; kill/restart
   on timeout or crash. In-process Harbour was rejected: protected calls catch
   language errors, not hangs or native crashes.
2. Module contract. serverd loads one local .hrb via --auth_module; a named
   but broken module prevents startup. Only functions the worker explicitly
   registers are callable from the module (docs/auth-hrb-pure-functions.txt):
   pure Harbour functions plus SHA256/384/512, HMAC-MD5/256/384/512, base64/hex
   helpers and OADS_RANDOMBYTES. No file, RDD, shell, network, loader, macro,
   sleep or pointer functions. A module calling anything else fails to load.
3. Module file protection enforced by the worker before load. Linux: chmod 600.
   Windows: ACL must contain exactly 3 entries - the server account (R),
   SYSTEM (R), Administrators (R); anything more (for example BUILTIN\Users or
   Authenticated Users, which a normal file save adds) is refused. Removal of
   direct entries needs icacls /remove; /inheritance:r alone keeps them.
4. V1 challenge auth. 32-byte CSPRNG challenge per attempt, bound to one
   encrypted transport and a 30 s deadline; the module sees challenge + opaque
   blob and returns allow with credentials or deny; OpenADS never parses the
   private blob. Documented in docs/HRB_CHALLENGE_AUTH_SPIKE.md.
5. V2 mutual auth (src/network/mutual_auth.h, session.cpp, client.cpp). The
   server proves itself to the client before the client sends its proof. Both
   sides compute HMAC-SHA256 over one transcript: the literal
   "OpenADS-mutual-v2" followed by u32-LE length-prefixed fields nonce,
   TLS channel binding (RFC 8446 exporter, 32 bytes), firm selector and
   requested path. Server proof is "SERVER:" + HMAC("server"\0 transcript);
   client proof is "CLIENT:" + HMAC("client"\0 transcript). The client verifies
   the server proof locally before completing. A module carrying both V2 hooks
   makes the server refuse V1-only logins.
6. TLS 1.3 only on the auth listener, with a server identity generated in
   memory at startup (self-signed, unverified by design). No certificate
   administration. Auth mode refuses explicit --tls_cert and --http_port on
   that listener. Channel binding pins the proofs to the TLS session, so a
   relay cannot replay them on a different connection.
7. Client API (src/abi/ace_exports.cpp, contrib/oads_hb/oads_hb.c):
   OAds_BeginAuth / OAdsCompleteAuth / OAdsCancelAuth (V1) and
   OAdsBeginMutualAuth (V2, returns the server proof for local verification).
   Harbour wrappers OADS_BEGINAUTH, OADS_BEGINMUTUAL, OADS_COMPLETEAUTH,
   OADS_CANCELAUTH, binary-safe. AdsConnect60 is unchanged; pending auth
   handles are never DB connections; the auth:// scheme never touches the
   insecure connection settings.
8. Startup policy (tools/serverd/main.cpp): non-loopback listeners refuse
   anonymous mode (need --auth_user, --auth_module or explicit
   --allow_anonymous); cleartext warnings when no TLS is configured; auth
   worker probe at startup (P call expects READY/READY2) with fail-closed
   startup on any failure.
9. Path jail (src/platform/path.cpp): the requested auth path is resolved
   under the configured data roots before the module is invoked; legacy path
   mode stays off by default (mtfix51 restoration, canonical containment and
   no-follow checks retained).
10. Client-side errors propagate with codes and messages (mtfix50): TCP
    failure 5172 "net_connect: ..." is raised before the TLS handshake;
    handshake and server denials carry their own texts, so field diagnosis
    does not need server access.

## What shipped, small items

- AuthWorker slot map: at most 4 concurrent calls per worker pool, busy
  answer instead of queueing (auth_worker.cpp).
- Pending-auth registry on the client: 64 pending cap, 30 s expiry sweep.
- Probe "P" handshake distinguishes V1-only modules (READY) from V2-capable
  modules (READY2).
- Worker call framing: single-letter op codes (P/B/M), u32-LE length-prefixed
  binary fields; module stdin wiped after read.
- Deny blob format "OAUTH"\1 + five zero bytes; allow blob carries the user
  and password lengths so the server can apply them to the session.
- Windows auth worker preserved under the portable POSIX blocked-signal drain
  (f96e625).
- Checksum manifest as plain text, warning-only length promotion fix
  (ba1140c).
- CLI trace hook for auth frames, off by default.
- serverd prints worker concurrency and probe result at startup; the worker
  itself stays silent on refusal by design (no detail leak to the module
  path).
- 22 focused unit test files (88 assertions) plus 132 root-mode TCP/auth
  matrix rows; test-only temp fixtures avoid pinned GCC 10.2
  temp_directory_path.

## Regression position vs plain 1.09.74

- Auth is off unless --auth_module is passed; without it, listener behavior
  is unchanged.
- AdsConnect60 and the RDD/engine table logic are untouched (scope:
  src/network, src/abi/ace_exports.cpp auth entry points, src/platform/path.cpp
  jail, tools/serverd, contrib/oads_hb, tests, docs).
- Owner-tested before merge (M5) and again after merge on the mtfix52 kits:
  loopback dummy key 2026-10-19 00:32 CDT, loopback real key 01:06 CDT,
  WAN EC2 01:50 CDT.
- Speed gate from the mtfix51 review: 30 whole-flow samples each, mtfix49
  median 473.30 ms, mtfix51 median 480.80 ms.

## Field references

- Session failure log with exact fix commands:
  docs/openads-v2-auth-test-2026-10-19.md (loopback + EC2, 8 failures F1-F8).
- V1 design ground rules: docs/HRB_CHALLENGE_AUTH_SPIKE.md.
- Worker-callable function surface: docs/auth-hrb-pure-functions.txt.
- Pre-merge test evidence: docs/MTFIX51_TESTED_MILESTONE.txt.
