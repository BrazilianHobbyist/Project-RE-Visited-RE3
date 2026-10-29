#!/usr/bin/env python3
"""test_settings_e2e.py: launcher choices -> settings.toml -> what the game applies.

For each case the real launcher (in a sandbox) is driven through its UI and
writes settings.toml. That file is put in the run folder (the player's own is
backed up and restored), the game runs the smoke route, and the effect is
checked from the game itself:

  re3_settings (debug server)  options as the process applied them
  ws_layers                    16:9 Zoomed actually composed frames
  geom_correction              PGXP geometry / texture correction
  video_info                   render scale (quality)
  stdout.log                   FXAA
  room capture                 side bars (4:3) or none (16:9)

  py -3 tools\\test_settings_e2e.py [--launcher launcher\\Resident_Evil_3_Launcher.exe]

Writes captures\\settings_e2e_<time>\\result.txt.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
WORKSPACE = os.path.dirname(REPO)
sys.path.insert(0, HERE)
import smoke_test as st  # noqa: E402
import test_launcher_sharp_font as tl  # noqa: E402

from PIL import Image  # noqa: E402

CB_SETCURSEL, WM_COMMAND, CBN_SELCHANGE = 0x014E, 0x0111, 1
COMBO = {"display": 1006, "aspect": 1005, "quality": 1008, "controls": 1010}
CHK_FAST_DOORS = 1037
ADV = {"pgxp": 1011, "texture": 1012, "fxaa": 1015, "spu": 1014, "skip_fmv": 1013, "dinput": 1038}
ID_BTN_ADV_OK = 1041
ID_BTN_CHEATS, ID_CHEAT_HEALTH, ID_CHEATS_SAVE = 1026, 1030, 1027

# UI actions and what the game must show. Combo indices follow the launcher:
# display 0 window / 1 fullscreen; aspect 0 stretched / 1 4:3 / 2 zoomed;
# quality 0 1080p / 1 4K / 2 1440p / 3 720p / 4 native; controls 0 classic / 1 modern.
CASES = [
    ("defaults", {}, set(), False, {
        "settings": {"fast_doors": 1, "modern_controls": 0, "ws_layers": 0, "geometry_correction": 0, "auto_skip_fmv": 0},
        "bars": False, "composed": False, "fxaa": True, "scale": 5}),
    ("zoomed_modern_tweaks", {"aspect": 2, "quality": 2, "controls": 1, "display": 0},
     {"pgxp", "fxaa", "skip_fmv", "spu", "dinput"}, True, {
        "settings": {"ws_layers": 1, "modern_controls": 1, "geometry_correction": 1, "auto_skip_fmv": 1,
                     "spu_hq": 1, "directinput_joysticks": 1, "fullscreen": 0, "cheats.health": 1},
        "bars": False, "composed": True, "fxaa": False, "scale": 6}),
    ("classic_43_no_doors", {"aspect": 1, "quality": 3}, set(), False, {
        "settings": {"ws_layers": 0, "fast_doors": 0},
        "bars": True, "composed": False, "fxaa": True, "scale": 3}),
]


def drive_launcher(exe, box, combos, adv_toggles, cheat_health, fast_doors_off):
    la = tl.Launcher(exe, box)
    u = tl.user32
    for name, idx in combos.items():
        h = u.GetDlgItem(la.hwnd, COMBO[name])
        u.SendMessageW(h, CB_SETCURSEL, idx, 0)
        u.SendMessageW(la.hwnd, WM_COMMAND, (CBN_SELCHANGE << 16) | COMBO[name], h)
        time.sleep(0.2)
    if fast_doors_off:
        la.click(la.hwnd, CHK_FAST_DOORS)
    if adv_toggles:
        adv = la.advanced()
        for t in adv_toggles:
            la.click(adv, ADV[t])
        la.click(adv, ID_BTN_ADV_OK)
    if cheat_health:
        u.PostMessageW(u.GetDlgItem(la.hwnd, ID_BTN_CHEATS), tl.BM_CLICK, 0, 0)
        ch = tl.wait_window(la.proc.pid, "RE3CheatsWindowClass", 5.0)
        la.click(ch, ID_CHEAT_HEALTH)
        la.click(ch, ID_CHEATS_SAVE)
    la.close()


def side_bars(png):
    im = Image.open(png).convert("L")
    w, h = im.size
    band = im.crop((0, h // 4, w // 10, 3 * h // 4))
    centre = im.crop((w // 3, h // 4, 2 * w // 3, 3 * h // 4))
    mean = lambda i: sum(i.get_flattened_data()) / (i.size[0] * i.size[1])  # noqa: E731
    return mean(band) < 2.0 and mean(centre) > 8.0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--launcher", default=os.path.join(REPO, "launcher", "Resident_Evil_3_Launcher.exe"))
    args = ap.parse_args()
    stamp = dt.datetime.now().strftime("%Y-%m-%d_%H%M%S")
    root = os.path.join(WORKSPACE, "captures", f"settings_e2e_{stamp}")
    os.makedirs(root)
    run_settings = os.path.join(st.PCPORT, "settings.toml")
    backup = open(run_settings, "rb").read()
    results = []

    def check(case, name, ok, detail=""):
        results.append((f"{case}: {name}", bool(ok)))
        print(f"{'OK  ' if ok else 'FAIL'} {case}: {name} {detail}")

    try:
        for case, combos, adv, cheat, expect in CASES:
            print(f"== {case}")
            box = os.path.join(root, case, "launcher")
            os.makedirs(box)
            exe = os.path.join(box, "Resident_Evil_3_Launcher.exe")
            shutil.copy2(args.launcher, exe)
            open(os.path.join(box, "Resident_Evil_3__Nemesis_Recompiled.exe"), "wb").close()
            shutil.copy2(os.path.join(REPO, "game.toml"), box)
            shutil.copy2(os.path.join(REPO, "settings.default.toml"), os.path.join(box, "settings.toml"))
            drive_launcher(exe, box, combos, adv, cheat, case == "classic_43_no_doors")
            shutil.copy2(os.path.join(box, "settings.toml"), run_settings)
            out = os.path.join(root, case, "game")
            subprocess.run([sys.executable, os.path.join(HERE, "smoke_test.py"), "--keep-open", "--out", out],
                           capture_output=True)
            st.GAME_PROC = None
            got = st.send({"cmd": "re3_settings"})
            ws = st.send({"cmd": "ws_layers"})
            geo = st.send({"cmd": "geom_correction"})
            vi = st.send({"cmd": "video_info"})
            st.send({"cmd": "quit"})
            time.sleep(3)
            for key, want in expect["settings"].items():
                v = got
                for part in key.split("."):
                    v = v.get(part, {}) if isinstance(v, dict) else None
                check(case, f"{key} = {want}", v == want, f"(game: {v})")
            check(case, "16:9 Zoomed composed" if expect["composed"] else "no composed frames",
                  (ws.get("composed", 0) > 0) == expect["composed"], f"(composed {ws.get('composed')})")
            room = [f for f in os.listdir(out) if f.endswith("_room.png")]
            if room:
                bars = side_bars(os.path.join(out, room[0]))
                check(case, "side bars in the room" if expect["bars"] else "no side bars in the room",
                      bars == expect["bars"])
            else:
                check(case, "room capture", False, "(missing)")
            log = open(os.path.join(out, "stdout.log"), encoding="utf-8", errors="replace").read()
            m = re.search(r"antialiasing (on|off)", log)
            check(case, f"FXAA {'on' if expect['fxaa'] else 'off'}", m and (m.group(1) == "on") == expect["fxaa"],
                  f"({m.group(0) if m else 'no log line'})")
            check(case, f"render scale {expect['scale']}x", vi.get("hr_scale") == expect["scale"],
                  f"(game: {vi.get('hr_scale')}x)")
            check(case, "texture correction on", geo.get("texture_correction") == 1)
            rep = json.load(open(os.path.join(out, "report.json")))
            check(case, "smoke route", rep.get("result") == "ok")
    finally:
        open(run_settings, "wb").write(backup)
        print("player settings restored:", open(run_settings, "rb").read() == backup)

    with open(os.path.join(root, "result.txt"), "w", encoding="utf-8") as f:
        for name, ok in results:
            f.write(f"{'OK  ' if ok else 'FAIL'} {name}\n")
    failed = [n for n, ok in results if not ok]
    print(f"{len(results) - len(failed)}/{len(results)} OK; {root}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
