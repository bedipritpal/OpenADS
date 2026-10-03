Built for Pritpal Bedi - bedipritpal/OpenADS, forked from FiveTechSoft/OpenADS.

# v1.09.72-mtfix27 - no-symlinks / no-escape data jail test build

Test build only, not a production recommendation. Use matching client/server kits and back up binaries and data before testing. Fork main and upstream are unchanged by this test build. Pritpal Bedi's application testing is the merge gate.

## Diagnosis and credit

Pritpal Bedi set the standing rule on September 27: "for ADSCDX purpose no symlinks, EVER." A security review of the fork (October 3, run against the published mtfix26 kit on a private loopback fixture) verified live that the rule was not actually enforced: the server accepted both a "../" table path escaping the data folder and a table symlink pointing outside it. The login directory was protected; the table open itself was not. This build enforces the rule for real.

## What changed

Server-side table and companion opens now go through a data jail installed once at startup from the --data roots (including each extra listener port's own root):

- A table path must stay inside its connection's data root, checked canonically at resolve time. A "../" escape or a symlink hop outside is refused with a REFUSED line in the audit log instead of opening.
- On Linux/macOS the open itself walks each path component from the anchored root with no-follow, so a symlink anywhere in the path - the file or any directory above it - fails the open. There is no check-then-open gap.
- On Windows directory handles pin each checked component without share-delete. The leaf is opened as a reparse point, never its target, and verified before any truncation. Symlinks, junctions and mount points are refused. Windows runtime validation comes from release CI, not this Linux machine.
- Create paths are covered too: creating over an existing symlink is refused and the link's target is left untouched. New table files keep their previous 0644 behavior.
- Only openads_serverd activates the jail. The ace client DLL never sets it; local/client containment, table sniffing and write paths retain their prior behavior when the jail is off. No management/auth policy changes are included.
- Configured roots themselves must be real directories without symlink/reparse components. Server startup fails if a root cannot be anchored.
- Direct table-header sniffing and temp/copy/restructure writes use no-follow handles while the server jail is active. This is table-data hardening, not a claim that all other server file operations (ZIP, backup, generic AdsF* operations) have been hardened.

One operational note: this is a strict never-follow. Old layouts where "<bag>.cdx" is a symlink to the real "<bag>.z01" (the Sep-5 links on the Lightsail server) will now be REFUSED. Replace those links with real copies before running this build.

## Regression tests

New file_jail tests prove: normal opens inside the root work (DBF plus companion .cdx/.z01/.fpt), a ".." that stays inside the root still opens, a "../" escape is refused and creates nothing, an outside absolute path is refused, a symlinked table file is refused, a symlinked intermediate directory is refused, a create over a symlink is refused with the target's bytes intact, the legacy .cdx -> .z01 link is refused, and clearing the jail restores legacy local behavior. The full existing suite runs unchanged.

## Local validation

Linux local build of the latest sources passed: openads_unit_tests and openads_serverd compiled; 10 jail/server-only tests passed (70 assertions); full ctest passed all 6 targets, including the slow suite and 30-run network-pool repeat. Local compilation used GCC 11 at -O0 with warnings-as-errors disabled because this small test machine cannot compile the large ACE source at -O3 and the fork has pre-existing GCC warnings. The guarded apply-patch workflow separately builds/tests on its standard CI toolchain before it writes the test branch. Windows/macOS execution is not claimed from this Linux result.

Live private-loopback check of this build: a normal DBF opened; the same ../ escape and outside-pointing table-link cases accepted by mtfix26 were refused. Authentication behavior was unchanged. The management snapshot issue is still present and is not addressed by this table-data patch.
