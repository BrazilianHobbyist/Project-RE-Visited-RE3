#!/usr/bin/env python3
"""test_launcher_sharp_font.py: drive the real launcher through the sharp-font flow.

Runs the launcher in a sandbox folder (a fake game exe, the repository's
default settings, the bundled hd-textures package), so the run folder is never
touched. The window is driven with Win32 messages:

  1. tick "Sharp font" and save  -> state.toml points the HD texture loader at
                                    mods/hd-pack/sharp-font, dump on, no
                                    replacements yet
  2. drop font dumps in, reopen  -> the launcher upscales every font atlas 4x,
                                    deletes non-font dumps and turns dump off
  3. untick and save             -> the hd-textures blocks leave state.toml

  py -3 tools\\test_launcher_sharp_font.py --dumps <pack>\\dumps

--dumps is a folder of real dumps (texupload-*.png) from a game session, e.g.
the one smoke_test.py --hd-pack <dir> --hd-dump writes. The sandbox goes to
<workspace>\\captures (it holds game-derived images).
"""

from __future__ import annotations

import argparse
import ctypes
import datetime as dt
import glob
import os
import shutil
import subprocess
import sys
import time
from ctypes import wintypes

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
WORKSPACE = os.path.dirname(REPO)

CLASS = "RE3RecompLauncherClass"
ID_BTN_SAVE = 1002
ID_CHK_SHARP_FONT = 1039
BM_GETCHECK, BM_CLICK, WM_CLOSE, WM_GETTEXT = 0x00F0, 0x00F5, 0x0010, 0x000D
FONT = "564BA4ED51F23272"

user32 = ctypes.WinDLL("user32", use_last_error=True)
user32.FindWindowW.restype = wintypes.HWND
user32.GetDlgItem.restype = wintypes.HWND
user32.SendMessageW.restype = ctypes.c_ssize_t
user32.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
EnumProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def wait_window(pid, cls, limit=15.0):
    found = []

    def cb(hwnd, _):
        p = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        name = ctypes.create_unicode_buffer(64)
        user32.GetClassNameW(hwnd, name, 64)
        if p.value == pid and name.value == cls and user32.IsWindowVisible(hwnd):
            found.append(hwnd)
        return True
    t0 = time.time()
    while time.time() - t0 < limit:
        found.clear()
        user32.EnumWindows(EnumProc(cb), 0)
        if found:
            return found[0]
        time.sleep(0.2)
    return None


def text(hwnd):
    buf = ctypes.create_unicode_buffer(512)
    user32.SendMessageW(hwnd, WM_GETTEXT, 512, ctypes.addressof(buf))
    return buf.value


class Launcher:
    def __init__(self, exe, cwd):
        self.proc = subprocess.Popen([exe], cwd=cwd)
        self.hwnd = wait_window(self.proc.pid, CLASS)
        if not self.hwnd:
            self.proc.kill()
            raise SystemExit("launcher window did not appear")
        time.sleep(0.5)
        self.chk = user32.GetDlgItem(self.hwnd, ID_CHK_SHARP_FONT)

    def checked(self):
        return user32.SendMessageW(self.chk, BM_GETCHECK, 0, 0) == 1

    def label(self):
        return text(self.chk)

    def toggle(self):
        user32.PostMessageW(self.chk, BM_CLICK, 0, 0)
        time.sleep(0.3)

    def save(self):
        """Save shows a message box; dismiss it."""
        user32.PostMessageW(user32.GetDlgItem(self.hwnd, ID_BTN_SAVE), BM_CLICK, 0, 0)
        box = wait_window(self.proc.pid, "#32770", 10.0)
        if box:
            user32.PostMessageW(box, WM_CLOSE, 0, 0)
        time.sleep(0.3)
        return box is not None

    def close(self):
        user32.PostMessageW(self.hwnd, WM_CLOSE, 0, 0)
        try:
            self.proc.wait(10)
        except subprocess.TimeoutExpired:
            self.proc.kill()


def state(box):
    path = os.path.join(box, "mods", "state.toml")
    return open(path, encoding="utf-8").read() if os.path.exists(path) else ""


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--launcher", default=os.path.join(REPO, "launcher", "Resident_Evil_3_Launcher.exe"))
    ap.add_argument("--dumps", required=True, help="folder with real texupload dumps")
    args = ap.parse_args()

    stamp = dt.datetime.now().strftime("%Y-%m-%d_%H%M%S")
    box = os.path.join(WORKSPACE, "captures", f"launcher_sharp_font_{stamp}")
    os.makedirs(box)
    exe = os.path.join(box, "Resident_Evil_3_Launcher.exe")
    shutil.copy2(args.launcher, exe)
    open(os.path.join(box, "Resident_Evil_3__Nemesis_Recompiled.exe"), "wb").close()
    shutil.copy2(os.path.join(REPO, "game.toml"), box)
    shutil.copy2(os.path.join(REPO, "settings.default.toml"), os.path.join(box, "settings.toml"))
    shutil.copytree(os.path.join(REPO, "build", "mods", "bundled", "psx.enhancement.hd-textures"),
                    os.path.join(box, "mods", "bundled", "psx.enhancement.hd-textures"))
    results = []

    def check(name, ok, detail=""):
        results.append((name, bool(ok)))
        print(f"{'OK  ' if ok else 'FAIL'} {name} {detail}")

    pack = os.path.join(box, "mods", "hd-pack", "sharp-font")

    print("1. tick Sharp font and save")
    la = Launcher(exe, box)
    check("starts unchecked", not la.checked())
    la.toggle()
    check("label says to play then reopen", "reopen the launcher" in la.label(), repr(la.label()))
    check("save dialog", la.save())
    la.close()
    st = state(box)
    check("state.toml enables hd-textures", 'id = "hd-textures"' in st and "enabled = true" in st)
    check("dump on until the font exists", 'dump = "true"' in st)
    check("pack is mods/hd-pack/sharp-font", "/mods/hd-pack/sharp-font" in st)
    check("no replacements yet", not glob.glob(os.path.join(pack, "replacements", "*.png")))

    print("2. game session dumps -> reopen")
    for f in glob.glob(os.path.join(args.dumps, "texupload-*.png")):
        shutil.copy2(f, os.path.join(pack, "dumps"))
    la = Launcher(exe, box)
    check("still checked", la.checked())
    check("label says it is on", "ON" in la.label(), repr(la.label()))
    la.close()
    st = state(box)
    check("dump off once the font exists", 'dump = "false"' in st)
    repl = sorted(glob.glob(os.path.join(pack, "replacements", "*.png")))
    check("font replacements written", any(FONT in r for r in repl), f"{len(repl)} files")
    for r in repl:
        src = Image.open(os.path.join(pack, "dumps", os.path.basename(r)))
        dst = Image.open(r)
        check(f"4x {os.path.basename(r)[13:46]}", dst.size == (src.width * 4, src.height * 4),
              f"{src.size} -> {dst.size}")
    left = [os.path.basename(f) for f in glob.glob(os.path.join(pack, "dumps", "*.png"))]
    check("only font dumps kept", left and all(FONT in f or "A054F93CA3CFD0A5" in f for f in left),
          f"{len(left)} kept")

    print("3. untick and save")
    la = Launcher(exe, box)
    la.toggle()
    la.save()
    la.close()
    st = state(box)
    check("hd-textures gone from state.toml", "hd-textures" not in st, repr(st[:120]))

    with open(os.path.join(box, "result.txt"), "w", encoding="utf-8") as f:
        for name, ok in results:
            f.write(f"{'OK  ' if ok else 'FAIL'} {name}\n")
    failed = [n for n, ok in results if not ok]
    print(f"{len(results) - len(failed)}/{len(results)} OK; sandbox: {box}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
