# OpenADS V2 mutual auth - first end-to-end test with own .hrb

Auth design and end-to-end testing by Pritpal Bedi on the bedipritpal/OpenADS fork of FiveTechSoft/OpenADS.

Session log, 2026-10-18 20:18 to 2026-10-19 01:52 (CDT).
Build: openads_serverd 1.09.74-mtfix52-mutual-auth-test. Module: openads_vouch_auth.hrb built from openads_vouch_auth.prg (one file, server half under -DHRB, Vouch half without). Environments: loopback on Windows PC (127.0.0.1:6262) and EC2 us-west-1 (13.56.76.69:6262, console-only access, no pem). Test credentials are the dummy pair usr:pw; no real keys or passwords appear in this document.

## Milestones

- M1  2026-10-18 20:29  Single-file openads_vouch_auth.prg in place (server functions OADS_AUTH_SERVER_V2 / OADS_AUTH_VERIFY_V2 plus the Vouch login function). Verified on Linux Harbour 3.2.1: compiles to .hrb, serverd loads it.
- M2  2026-10-18 22:19  openads_vouch_auth.hbp written; hbmk2 build makes the .hrb with only the 2 server functions.
- M3  2026-10-18 22:46  .hrb ACL locked to exactly 3 entries (owner, SYSTEM, Administrators).
- M4  2026-10-19 00:00  serverd starts clean on loopback (after F1-F3 below).
- M5  2026-10-19 00:32  Loopback V2 login passes with dummy key in both halves.
- M6  2026-10-19 00:47  Own hbmk2-built .hrb accepted by the worker. Root cause of the original probe failure found (F2).
- M7  2026-10-19 01:06  Loopback V2 login passes with the real key in both halves. Production grade.
- M8  2026-10-19 01:13  California login passes - later shown to be against the kit mock module, not the new .hrb (F7).
- M9  2026-10-19 01:41  New .hrb moved to the EC2 box through the AWS console terminal via base64 paste (F8).
- M10 2026-10-19 01:50  California restarted with own .hrb; Vouch login passes over auth://13.56.76.69:6262/C:/. V2 proven end to end on WAN with the real module.

## Failures and fixes

### F1  2026-10-18 22:48  "Auth worker/module probe failed" - wrong --auth_module path
Symptom: serverd prints "Auth worker concurrency: 4 (no queue)." then "Auth worker/module probe failed."
Cause: the start line pointed at C:\Openads_1\openads_vouch_auth.hrb while the protected file was C:\openads_1\build\openads_vouch_auth.hrb (\build\ missing).
Fix: start with the full path:

    openads_serverd --config openads.ini --auth_module C:\openads_1\build\openads_vouch_auth.hrb --auth_user usr:pw

### F2  2026-10-18 22:53 - 2026-10-19 00:47  probe failed with the right path - .hrb held 4 functions
Symptom: probe failed again on the correct path. Direct worker probe (probe_hrb.ps1) printed only "exit=1 stdout=[] stderr=[]" - the worker refuses the module silently, by design.
Wrong theory, tested and rejected: Harbour compiler version (PC has 3.2.0dev r1901140418, worker built with 3.2.1dev r2609292008). Harbour's loader accepts both .hrb formats; a matching-compiler build was sent and the version question was closed. The compiler was never the problem.
Actual cause, found 2026-10-19 00:42: a correct server build prints "Lines 93, Functions/Procedures 2" (or 70/2 after cleanup). The failing build printed "Lines 98, Functions/Procedures 4". Two extra hardening functions had been added to the .prg; they called symbols the worker does not publish, so the loader refused the whole module.
Fix: remove the extra functions so only the 2 server functions compile, rebuild in a fresh folder, confirm the hbmk2 counts:

    hbmk2 openads_vouch_auth.hbp
    :: must print: Lines 70, Functions/Procedures 2

Rebuild accepted at 00:47: serverd prints "listening on 127.0.0.1:6262".

### F3  2026-10-18 23:47  ACL: extra entries survive /inheritance:r
Symptom: icacls listing on the v52_test copy showed 5 entries. The loader allows exactly 3 accounts on the .hrb: the serverd account, SYSTEM, Administrators.
Cause: BUILTIN\Users:(RX) and Authenticated Users were set directly on the file, so /inheritance:r did not remove them.
Fix: remove the two extra entries explicitly (hrb_acl_fix.cmd), then the standard ACL set:

    icacls "C:\openads_1\build\v52_test\openads_vouch_auth.hrb" /inheritance:r
    icacls "C:\openads_1\build\v52_test\openads_vouch_auth.hrb" /grant:r "%USERNAME%:R"
    icacls "C:\openads_1\build\v52_test\openads_vouch_auth.hrb" /grant:r "*S-1-5-18:R"
    icacls "C:\openads_1\build\v52_test\openads_vouch_auth.hrb" /grant:r "*S-1-5-32-544:R"
    icacls "C:\openads_1\build\v52_test\openads_vouch_auth.hrb"

The last listing must show exactly the 3 entries. Run in cmd, not PowerShell; watch that chat apps do not eat the * before S-1-5. Rule: redo the ACL after every rebuild, the new file inherits folder defaults again.

### F4  2026-10-19 00:05  V2Login returns 5172 - wrong port
Symptom: 5172 = plain TCP connect failure, before TLS or any login step.
Cause: the test line still used port 6264 from a previous run; serverd was listening on 6262.
Fix: point the call at the server actually running:

    V2Login( "auth://127.0.0.1:6262/C:/", "fixture-firm", "C:/" )

Check what is listening with:

    netstat -ano | findstr :6262

### F5  2026-10-19 00:08 - 00:32  V2Login returns 0, "failed on equal bytes" - key mismatch
Symptom: the server answered and Vouch received its proof, but the proof Vouch computes does not equal the one the .hrb sent.
Cause (suspected, not isolated): the two halves did not hold the same key. The two proofs can never match.
Fix: same key in both halves. For the loopback milestone the Vouch side was set back to the dummy key; login passed at 00:32. Rule: the key never travels through chat; each side holds its own copy.

### F6  2026-10-19 00:48 - 01:06  "CompleteAuth denied"
Symptom: Vouch accepted the server's proof (client half correct), then the server denied: OADS_AUTH_VERIFY_V2 returned deny.
Cause: not confirmed. Suspected key mismatch between the two halves (line 53 of the .prg showed the DUMMY word; the owner states the real key is built on the fly, so this word is not the proof). User/password were checked: the .hrb returns usr:pw, matching --auth_user usr:pw.
Fix that worked: rebuild the .hrb with the matching key on the server half, redo the ACL (F3 rule), restart serverd, rerun the login:

    openads_serverd --config openads.ini --auth_module C:\Openads_1\build\v52_test\hrb_3\openads_vouch_auth.hrb --auth_user usr:pw

Loopback passed with the real key in both halves at 01:06.

### F7  2026-10-19 01:13 - 01:45  California "pass" was the kit mock module, not own .hrb
Symptom: login on auth://13.56.76.69:6262/C:/ succeeded before the new .hrb was ever copied to the box.
Cause: the daemon there is started by ~/start-openads.sh, which runs the kit's run-mock-test.sh, and that script hard-codes the mock module:

    grep -n KIT= start-openads.sh
    :: 8:KIT=/tmp/openads-1.09.74-mtfix52-linux-test-linux-x64
    grep -n auth /tmp/openads-1.09.74-mtfix52-linux-test-linux-x64/run-mock-test.sh
    :: 13:exec ./openads_serverd --config ./openads.ini --http_port 0 --auth_module "$PWD/mutual-test-only.hrb" --auth_user usr:pw

Fix: put own .hrb in place of mutual-test-only.hrb and restart (M9/M10 commands below). Reachability check used before the login:

    Test-NetConnection 13.56.76.69 -Port 6262

### F8  2026-10-19 01:23 - 01:41  moving the .hrb to a console-only EC2 box
Symptom: no pem exists for this box (launched with a key that does not match the local ppk), so no scp/ssh; only the AWS console terminal. First attempt, echo '<base64>' | base64 -d > file, broke: the paste disturbed the quoting and the shell sat at a > continuation prompt.
Fix: cancel with Ctrl+C, then the two-step paste that cannot break quoting:

    [Convert]::ToBase64String([IO.File]::ReadAllBytes("C:\Openads_1\build\v52_test\hrb_3\openads_vouch_auth.hrb")) | Set-Clipboard   :: on the PC, PowerShell

    cat > ~/h.b64        :: on the EC2 console: type, Enter, Ctrl+V, Enter, Ctrl+D
    base64 -d ~/h.b64 > ~/openads_vouch_auth_new.hrb
    ls -l ~/openads_vouch_auth_new.hrb

### M10 commands  2026-10-19 01:46 - 01:50  swap and restart on California

    cp /tmp/openads-1.09.74-mtfix52-linux-test-linux-x64/mutual-test-only.hrb ~/mutual-test-only.hrb.bak
    cp ~/openads_vouch_auth_new.hrb /tmp/openads-1.09.74-mtfix52-linux-test-linux-x64/mutual-test-only.hrb
    ./start-openads.sh stop
    ./start-openads.sh

Start prints "serverd running, detached" and the listening check shows 0.0.0.0:6262. Vouch login on auth://13.56.76.69:6262/C:/ passed at 01:50.

## Result and notes

- V2 one-trip mutual proof passes end to end on loopback and on WAN (N. California EC2), with own .hrb and the real key in both halves.
- Key design note (owner): the real key is built on the fly and is always different; the DUMMY word stays in the file on purpose, kept for hackers.
- Server behaviour worth remembering: the worker refuses a module silently (F2); the loader allows exactly 3 ACL accounts on the .hrb (F3); every login spawns a fresh worker, so replacing the .hrb is live on next login, but a running daemon keeps the module it started with - restart after a swap (M10).
- Open items parked from the same night: client version still prints c1.9a (separate hunt, unresolved); production deploy of the real vouch_auth.hrb to the Lightsail box still pending.
