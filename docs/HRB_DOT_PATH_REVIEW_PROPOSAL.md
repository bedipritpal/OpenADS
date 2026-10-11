# Dot-path compatibility proposal - REVIEW ONLY, NOT APPLIED

Prepared for Pritpal Bedi. No main, PR, release or package change.

## What is proven

Winlibs GCC10.2 x86 rejects `weakly_canonical(cwd / ".")` with
"No such file or directory". `weakly_canonical((cwd / ".").lexically_normal())`
succeeds. The same raw dot path succeeds on x64.

This made the server return 7077 "Auth path denied" before the HRB hook.
`RemoteConnection::begin_auth` currently hides that underlying error as 5000.
It was not a DLL calling-convention or TLS integrity failure.

Both architectures now pass the native Harbour DLL/static client, TLS
record-tamper/replay and worker-crash tests with a named existing data directory
and full absolute client path, without a product change:
https://github.com/bedipritpal/OpenADS/actions/runs/38150418804

These tests use a synthetic HRB. The owner's HRB is not tested here. TLS server
identity is unverified and an active TLS-terminating relay still succeeds.

## Do not apply full lexical normalization to untrusted paths

`lexically_normal()` removes both `.` and `..` without consulting the filesystem.
That can change the target when a preceding component is a symlink or Windows
junction. For example, `link/../file` must not be rewritten to `file` before
filesystem resolution: the real parent of the symlink target may be outside the
configured root. Removing the link could hide the component from no-follow
checks. A green dot-path test is not evidence that this rewrite is safe.

The proposed narrower compatibility change removes only components equal to
`.` from temporary paths used for canonical comparison. It never cancels a
preceding component with `..`; never folds case; never strips a drive/UNC root;
never turns a filesystem error into approval. The existing canonical-root
boundary check remains mandatory.

Keep the original joined spelling for `File::data_jail_active()` returns.
The jailed open layer must still see and reject original symlink/reparse and
parent-traversal components. On Windows it opens directories with
FILE_FLAG_OPEN_REPARSE_POINT and keeps handles; on POSIX the jailed open uses
its own no-follow walk. The proposed comparison workaround does not replace
those checks and does not fix any existing race outside that jailed open.

## Candidate diff (not applied to src/platform/path.cpp)

Add this helper in path.cpp's anonymous namespace:

```cpp
fs::path without_dot_components(const fs::path& input) {
    fs::path out;
    for (const auto& part : input) {
        if (part == fs::path(".")) continue;
        out /= part;
    }
    // Preserve a dot-only relative path as the current directory.
    return out.empty() ? fs::path(".") : out;
}
```

At the two canonicalization calls only:

```diff
-    fs::path canon = fs::weakly_canonical(combined, ec);
+    fs::path canon = fs::weakly_canonical(without_dot_components(combined), ec);
     if (ec) return std::nullopt;

-    fs::path root_canon = fs::weakly_canonical(root_p, ec);
+    fs::path root_canon = fs::weakly_canonical(without_dot_components(root_p), ec);
```

Do not change `combined`, `path_has_prefix`, error handling or the returned
original spelling when the file jail is active. Do not replace this helper
with full `lexically_normal()`.

## Limited preparation check

A small Linux filesystem check confirms the proposed dot-only helper preserves
`link/../file` resolution, while full lexical normalization changes it. It also
checks dot removal and retention of parent components. This is not a Windows
unit-test run, an actual jailed-open test or proof of the proposed native fix.

## Tests required before applying or rebuilding an owner package

- Matched Winlibs GCC10.2 x86 and x64: root `.` with client `.`, explicit absolute
  root with client `.`, dot components inside paths, root spelling ending `/.`,
  trailing separators, drive roots and UNC roots.
- Named directory/full absolute client path remains green. Relative child,
  missing leaf and multi-root behavior remain unchanged.
- Reject `..` escape, absolute outside-root path, sibling prefix such as
  `root-other`, and a symlink/junction pointing outside.
- Compare old and candidate results for `link/../file`: no lexical cancellation
  and no new admission. Test both inside-root and outside-root link targets.
- With the file jail active, preserve original `link` and `..` spelling so the
  no-follow open cannot be bypassed. Attempt actual reads/writes through those
  spellings and verify denial, not only resolver output.
- Run directory links/junctions on Windows even if unprivileged symlink creation
  is unavailable. Report skipped coverage explicitly, never as passed.
- Run the native Harbour DLL/static auth and tamper/replay/crash matrix again.

The helper and this test plan are a proposal, not a tested shipping fix. The
morning package remains unchanged. Until review and native verification, use
an existing named absolute server data root and the matching absolute client
path; avoid dot components.
