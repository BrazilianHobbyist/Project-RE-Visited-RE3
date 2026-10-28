#!/usr/bin/env python3
"""runtime_patches.py: the RE3 changes to the PSXRecomp runtime, one patch per theme.

patches/runtime/NN-<theme>.patch apply in name order on top of the submodule
commit. Each one was cut from the tree after the previous ones, so only the
whole sequence is meaningful. The expected tree is built in a temporary git
index (nothing in the submodule changes while checking).

  py -3 tools\\runtime_patches.py status   clean / applied / dirty (and what differs)
  py -3 tools\\runtime_patches.py ensure   apply the sequence on a clean submodule;
                                           build.ps1 runs this
  py -3 tools\\runtime_patches.py export NN-theme
                                           write the submodule's changes beyond the
                                           sequence as a new last patch
  py -3 tools\\runtime_patches.py rebuild NN-theme
                                           rewrite an existing patch so the
                                           sequence reproduces the submodule tree
                                           (only the named patch changes; later
                                           ones must still apply)

Patches are written as raw bytes from git (never through PowerShell, which
re-encodes them).
"""

from __future__ import annotations

import glob
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
SUB = os.path.join(REPO, "psxrecomp")
PATCH_DIR = os.path.join(REPO, "patches", "runtime")


def git(*args, env=None, check=True, inp=None):
    r = subprocess.run(["git", "-C", SUB, *args], capture_output=True, env=env, input=inp)
    if check and r.returncode:
        raise SystemExit(f"git {' '.join(args)}: {r.stderr.decode(errors='replace').strip()}")
    return r


def patches():
    return sorted(glob.glob(os.path.join(PATCH_DIR, "*.patch")))


def expected_tree(upto=None):
    """Tree id of HEAD + the patches (all, or those before `upto`)."""
    with tempfile.TemporaryDirectory() as tmp:
        env = dict(os.environ, GIT_INDEX_FILE=os.path.join(tmp, "index"))
        git("read-tree", "HEAD", env=env)
        for p in patches():
            if upto and os.path.basename(p) >= upto:
                break
            r = git("apply", "--cached", p, env=env, check=False)
            if r.returncode:
                raise SystemExit(f"{os.path.basename(p)} does not apply: "
                                 f"{r.stderr.decode(errors='replace').strip()}")
        return git("write-tree", env=env).stdout.decode().strip()


def worktree_diff(tree):
    """Diff from `tree` to the working tree (tracked + untracked, not ignored)."""
    with tempfile.TemporaryDirectory() as tmp:
        env = dict(os.environ, GIT_INDEX_FILE=os.path.join(tmp, "index"))
        git("read-tree", "HEAD", env=env)
        git("add", "-A", env=env)
        work = git("write-tree", env=env).stdout.decode().strip()
    return git("diff", "--binary", tree, work).stdout


def state():
    head = git("rev-parse", "HEAD^{tree}").stdout.decode().strip()
    if not worktree_diff(head):
        return "clean", b""
    rest = worktree_diff(expected_tree())
    return ("applied", b"") if not rest else ("dirty", rest)


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else "status"
    if cmd == "status":
        st, rest = state()
        print(st)
        if rest:
            print(git("apply", "--stat", "-", inp=rest).stdout.decode(), end="")
        return 0
    if cmd == "ensure":
        st, _ = state()
        if st == "clean":
            for p in patches():
                git("apply", p)
                print(f"[PATCH] {os.path.basename(p)}")
        elif st == "applied":
            print("[PATCH] runtime patches already applied")
        else:
            print("[!] psxrecomp has changes beyond patches/runtime "
                  "(development). Before committing: tools\\runtime_patches.py export|rebuild")
        return 0
    if cmd in ("export", "rebuild") and len(sys.argv) == 3:
        name = sys.argv[2].removesuffix(".patch") + ".patch"
        path = os.path.join(PATCH_DIR, name)
        if cmd == "export":
            if os.path.exists(path):
                raise SystemExit(f"{name} exists; use rebuild")
            rest = worktree_diff(expected_tree())
            if not rest:
                raise SystemExit("nothing beyond the current sequence")
            open(path, "wb").write(rest)
        else:
            if not os.path.exists(path):
                raise SystemExit(f"{name} does not exist")
            later = [p for p in patches() if os.path.basename(p) > name]
            base = expected_tree(upto=name)
            # Target = working tree minus the later patches, built in a temp index.
            with tempfile.TemporaryDirectory() as tmp:
                env = dict(os.environ, GIT_INDEX_FILE=os.path.join(tmp, "index"))
                git("read-tree", "HEAD", env=env)
                git("add", "-A", env=env)
                for p in reversed(later):
                    r = git("apply", "--cached", "-R", p, env=env, check=False)
                    if r.returncode:
                        raise SystemExit(f"{os.path.basename(p)} no longer applies on top "
                                         f"of the change; fold it in or reorder")
                target = git("write-tree", env=env).stdout.decode().strip()
            open(path, "wb").write(git("diff", "--binary", base, target).stdout)
        st, _ = state()
        print(f"{name} written; sequence now: {st}")
        return 0 if st == "applied" else 1
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
