#!/usr/bin/env python3
"""smoke_test.py: boot -> title -> load the save on card 1 -> walk, unattended.

Drives the debug-tools build (build_dbg) through the debug server on port 4370,
waiting on guest frame counts instead of wall-clock sleeps. It writes numbered
screenshots, stdout.log and report.json into the session folder. The
screenshots are the evidence: open them. The script only fails on hard errors
(no debug server, game exited, savestate not loaded, FPS far too low).

The player's memory cards are never touched: the game runs with --memcard-dir
pointing at a copy inside the session folder (cards, savestates and the
per-game options all live there).

  py -3 tools\\smoke_test.py                       full route from boot
  py -3 tools\\smoke_test.py --save-state 9        ...and save slot 9 in the room
                                                  (inside the session only)
  py -3 tools\\smoke_test.py --from-state 9 --state-session <session>
                                                  load that session's slot 9 and
                                                  run only the gameplay part
  py -3 tools\\smoke_test.py --boot-only 3600      boot and shoot every 300 frames

Sessions go to <workspace>\\captures\\smoke_<date>_<time> (outside the
repository: screenshots and RAM are game data).
"""

from __future__ import annotations

import argparse
import datetime as dt
import glob
import hashlib
import json
import os
import shutil
import socket
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
WORKSPACE = os.path.dirname(REPO)
PCPORT = os.path.join(WORKSPACE, "ResidentEvil3-PCPort")
sys.path.insert(0, os.path.join(REPO, "psxrecomp", "tools"))
from debug_client import query  # noqa: E402

# RE3's debug port (the runtime default). The Muppet Monster Adventure project
# on this machine uses 4390; this script only ever talks to the process it
# launched, and refuses to start when 4370 is already taken.
PORT = 4370
EXE_NAME = "Resident_Evil_3__Nemesis_Recompiled"
DBG_SRC = os.path.join(REPO, "build_dbg", EXE_NAME + ".exe")
DBG_EXE = os.path.join(PCPORT, EXE_NAME + "_debug.exe")
GAME_PROC = None

# RAM the modern controls already read (main.cpp, re3_screen_relative_move).
PLAYER = 0x800CCBC4    # player work struct
UI_FLAGS = 0x800D4314  # 0x00080808 set when the inventory / map / a message opens
                       # (measured 2026-10-27: it stays set after the inventory closes)
UI_MENU = 0x00080808

# Active-low pad words.
NONE = 0xFFFF
SELECT = 0xFFFE
START = 0xFFF7
UP = 0xFFEF
RIGHT = 0xFFDF
DOWN = 0xFFBF
LEFT = 0xFF7F
TRIANGLE = 0xEFFF
CIRCLE = 0xDFFF
CROSS = 0xBFFF
SQUARE = 0x7FFF
R1 = 0xF7FF


class GameExited(RuntimeError):
    pass


class StateLoadFailed(RuntimeError):
    pass


def port_in_use(port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(0.5)
        return s.connect_ex(("127.0.0.1", port)) == 0


def send(cmd, timeout=5.0):
    # Never send anything once our own process is gone: whatever answers on
    # the port then is not the game this script launched.
    if GAME_PROC is not None and GAME_PROC.poll() is not None:
        raise GameExited(f"game process exited (code {GAME_PROC.returncode})")
    try:
        return query("127.0.0.1", PORT, cmd, timeout=timeout)
    except Exception as e:  # server busy or gone
        return {"ok": False, "error": str(e)}


def frame():
    r = send({"cmd": "frame"})
    return r.get("frame", -1) if r and r.get("ok") else -1


def wait_frames(n, limit_s=120.0):
    """Wait until n more guest frames have run. Returns frames actually run."""
    start = frame()
    t0 = time.time()
    while time.time() - t0 < limit_s:
        cur = frame()
        if cur >= 0 and cur - start >= n:
            return cur - start
        time.sleep(0.05)
    return frame() - start


def state_request(op, slot, limit_s=15.0):
    """Save or load a savestate and confirm it. The savestate command only
    acknowledges the request; the outcome comes from savestate_status
    (completion generation + last_ok + last_op)."""
    before = send({"cmd": "savestate_status"})
    if not before.get("ok"):
        raise StateLoadFailed(f"savestate_status unavailable: {before}")
    r = send({"cmd": "savestate", "op": op, "slot": slot})
    if not r.get("ok"):
        raise StateLoadFailed(f"{op} slot {slot}: {r}")
    t0 = time.time()
    while time.time() - t0 < limit_s:
        s = send({"cmd": "savestate_status"})
        if (s.get("ok") and s.get("generation", 0) != before.get("generation")
                and not s.get("pending")):
            if s.get("last_ok") and s.get("last_op") == op:
                return s
            raise StateLoadFailed(f"{op} slot {slot}: {s}")
        time.sleep(0.1)
    raise StateLoadFailed(f"{op} slot {slot}: no result within {limit_s:.0f} s")


def present_shot(path, limit_s=5.0):
    """Capture what the window presents (aspect, internal resolution, PGXP all
    resolved). The request is staged and fulfilled on the next present."""
    before = send({"cmd": "present_shot_seq"}).get("seq", -1)
    r = send({"cmd": "present_shot", "path": path})
    if not r.get("ok"):
        return r
    t0 = time.time()
    while time.time() - t0 < limit_s:
        s = send({"cmd": "present_shot_seq"})
        if s.get("seq", before) != before:
            return {"ok": True} if s.get("wrote") else \
                {"ok": False, "error": "present_shot completed without a PNG"}
        time.sleep(0.03)
    return {"ok": False, "error": "present_shot timed out"}


class Run:
    def __init__(self, out):
        self.out = out
        self.shots = []
        self.n = 0

    def shot(self, label):
        self.n += 1
        path = os.path.join(self.out, f"{self.n:02d}_{label}.png")
        r = present_shot(path)
        f = frame()
        self.shots.append({"label": label, "frame": f, "path": path,
                           "ok": bool(r.get("ok")), "error": r.get("error")})
        print(f"  [{f:6d}] {label}: {'ok' if r.get('ok') else r}")

    def press(self, buttons, frames=6, then=30):
        send({"cmd": "press", "buttons": f"0x{buttons:04X}", "frames": frames})
        wait_frames(frames + then)


def prepare_memcards(out):
    """Copy the player's cards and savestates into the session; the game only
    ever writes there."""
    dst = os.path.join(out, "memcards")
    os.makedirs(dst, exist_ok=True)
    src = os.path.join(PCPORT, "saves")
    for name in ("card1.mcd", "card2.mcd"):
        if os.path.isfile(os.path.join(src, name)):
            shutil.copy2(os.path.join(src, name), dst)
    for sub in glob.glob(os.path.join(src, "*", "")):
        if os.path.basename(os.path.dirname(sub)).startswith("_"):
            continue
        rel = os.path.relpath(sub, src)
        shutil.copytree(sub, os.path.join(dst, rel), dirs_exist_ok=True)
    return dst


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def sync_debug_exe():
    """Same rule as run_debug.ps1: the run folder gets the build_dbg exe under
    the _debug name, so it is never mistaken for the release exe."""
    if not os.path.isfile(DBG_SRC):
        raise SystemExit(f"debug exe not found: {DBG_SRC} (run tools\\run_debug.ps1 -Build)")
    if not os.path.isfile(DBG_EXE) or sha256(DBG_SRC) != sha256(DBG_EXE):
        shutil.copy2(DBG_SRC, DBG_EXE)
        print(f"debug exe updated: {DBG_EXE}")


def bios_arg():
    cfg = os.path.join(PCPORT, "bios.cfg")
    if os.path.isfile(cfg):
        with open(cfg, encoding="utf-8-sig") as f:
            return f.readline().strip().replace("/", "\\")
    return "bios\\openbios.bin"


def stats(report, key):
    report[key] = {
        "frame": frame(),
        "overlay_loader": send({"cmd": "overlay_loader_status"}),
        "dispatch": send({"cmd": "dispatch_stats"}),
        "audio": send({"cmd": "audio_stats"}),
        "geom_correction": send({"cmd": "geom_correction"}),
    }


def gameplay(run, report):
    """Walk with the d-pad and the left stick, open and close the inventory.
    Checks read RAM: Jill must be controllable and move with both inputs, the
    inventory flag must rise, and she must walk again after it closes."""
    print("gameplay")
    t0, f0 = time.time(), frame()
    run.shot("room")
    p0 = player()
    check(report, "in_room", in_room(), f"player {p0}")
    send({"cmd": "set_input", "buttons": f"0x{UP:04X}"})
    wait_frames(90)
    run.shot("dpad_up")
    send({"cmd": "set_input", "buttons": f"0x{RIGHT:04X}"})
    wait_frames(40)
    send({"cmd": "set_input", "buttons": f"0x{UP:04X}"})
    wait_frames(90)
    run.shot("dpad_turn_up")
    p1 = player()
    send({"cmd": "set_input", "buttons": f"0x{NONE:04X}", "lx": 128, "ly": 0})
    wait_frames(90)
    run.shot("stick_up")
    send({"cmd": "set_input", "buttons": f"0x{NONE:04X}", "lx": 0, "ly": 128})
    wait_frames(90)
    run.shot("stick_left")
    send({"cmd": "clear_input"})
    wait_frames(30)
    p2 = player()
    report["gameplay_fps"] = round((frame() - f0) / (time.time() - t0), 2)

    def dist(a, b):
        return abs(a["x"] - b["x"]) + abs(a["z"] - b["z"]) if a and b else 0
    check(report, "dpad_moves", dist(p0, p1) > 200, f"moved {dist(p0, p1)}")
    check(report, "stick_moves", dist(p1, p2) > 200, f"moved {dist(p1, p2)}")

    flags0 = ui_flags()
    run.press(CIRCLE, then=90)
    run.shot("inventory")
    flags1 = ui_flags()
    run.press(TRIANGLE, then=90)
    run.shot("inventory_closed")
    menu = lambda f: f is not None and (f & UI_MENU) == UI_MENU  # noqa: E731
    check(report, "inventory_opens", not menu(flags0) and menu(flags1),
          f"ui 0x{flags0 or 0:08X} -> 0x{flags1 or 0:08X}")
    # The UI flag does not fall on close; Jill walking again is the proof.
    p3 = player()
    send({"cmd": "set_input", "buttons": f"0x{DOWN:04X}"})
    wait_frames(60)
    send({"cmd": "clear_input"})
    wait_frames(10)
    p4 = player()
    run.shot("walk_after_inventory")
    check(report, "inventory_closes", dist(p3, p4) > 200, f"moved {dist(p3, p4)} after closing")
    stats(report, "gameplay_stats")
    return 0 if all(c["ok"] for c in report["checks"]) else 1


def summary(report):
    """The few numbers worth reading first; the raw stats stay in report.json."""
    g = report.get("gameplay_stats") or {}
    ol, au, gc = g.get("overlay_loader", {}), g.get("audio", {}), g.get("geom_correction", {})
    tc = gc.get("texcorr", {})
    return {
        "boot_fps": report.get("boot_fps"),
        "gameplay_fps": report.get("gameplay_fps"),
        "overlay_dlls_loaded": ol.get("loads"),
        "audio_underruns": au.get("underruns"),
        "audio_mutes": au.get("mutes"),
        "texcorr_no_depth_pct": round(100.0 * tc["no_depth"] / tc["attempts"], 2)
        if tc.get("attempts") else None,
        "checks_failed": [c["check"] for c in report.get("checks", []) if not c["ok"]],
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--out", help="session folder (default: captures/smoke_<time>)")
    ap.add_argument("--boot-frames", type=int, default=2250,
                    help="frames before pressing on the title (it shows at ~2100; the attract demo starts at ~2900)")
    ap.add_argument("--boot-only", type=int, metavar="FRAMES",
                    help="only boot, shooting every 300 frames (to study the route)")
    ap.add_argument("--save-state", type=int, metavar="SLOT",
                    help="save a state in the loaded room into SLOT (session copy)")
    ap.add_argument("--from-state", type=int, metavar="SLOT",
                    help="skip boot and menus: load SLOT, then run the gameplay part")
    ap.add_argument("--state-session", metavar="DIR",
                    help="with --from-state: take the savestate from an earlier "
                         "session folder instead of the run folder's saves")
    ap.add_argument("--min-fps", type=float, default=50.0,
                    help="fail when gameplay runs slower than this")
    ap.add_argument("--keep-open", action="store_true",
                    help="leave the game running at the end")
    ap.add_argument("--window", default="1920x1080", metavar="WxH",
                    help="window size for the captures (small windows hide texture detail)")
    ap.add_argument("--hd-pack", metavar="DIR",
                    help="run with this HD texture pack (replacements on); the run "
                         "folder's mods/state.toml is swapped and restored afterwards")
    ap.add_argument("--hd-dump", action="store_true",
                    help="with --hd-pack: also dump the textures used into DIR/dumps")
    args = ap.parse_args()

    stamp = dt.datetime.now().strftime("%Y-%m-%d_%H%M%S")
    out = os.path.abspath(args.out or os.path.join(WORKSPACE, "captures", f"smoke_{stamp}"))
    os.makedirs(out, exist_ok=True)

    running = [p for p in subprocess.run(
        ["tasklist", "/FO", "CSV", "/NH"], capture_output=True, text=True).stdout.splitlines()
        if EXE_NAME in p]
    if running:
        print("an RE3 game is already open; close it first (it owns port 4370 and the cards)")
        return 2
    if port_in_use(PORT):
        print(f"port {PORT} is already in use by another process; refusing to start "
              f"(it would receive this script's input)")
        return 2

    sync_debug_exe()
    cards = prepare_memcards(out)
    if args.state_session:
        import_state(cards, args.state_session, args.from_state)
    log = open(os.path.join(out, "stdout.log"), "w", encoding="utf-8")
    cmdline = [DBG_EXE, "--bios", bios_arg(), "--debug-port", str(PORT),
               "--memcard-dir", cards]
    state_backup = swap_mod_state(args.hd_pack, args.hd_dump) if args.hd_pack else None
    try:
        proc = subprocess.Popen(cmdline, cwd=PCPORT, stdout=log, stderr=subprocess.STDOUT)
    except OSError:
        if state_backup is not None:
            restore_mod_state(state_backup)
        raise
    global GAME_PROC
    GAME_PROC = proc
    run = Run(out)
    report = {"time": stamp, "exe": DBG_EXE, "exe_sha256": sha256(DBG_EXE),
              "cmdline": cmdline, "port": PORT, "args": vars(args)}
    t_start = time.time()
    rc = 1
    try:
        for _ in range(60):
            time.sleep(0.5)
            if proc.poll() is not None or send({"cmd": "ping"}).get("ok"):
                break
        if proc.poll() is not None or not send({"cmd": "ping"}).get("ok"):
            report["error"] = f"no debug server (exit={proc.poll()})"
            return 1
        ww, _, wh = args.window.partition("x")
        report["window"] = send({"cmd": "window_size", "w": int(ww), "h": int(wh)})

        if args.boot_only:
            t0 = time.time()
            for i in range(0, args.boot_only, 300):
                wait_frames(300)
                run.shot(f"boot_f{i + 300}")
            report["boot_fps"] = round(args.boot_only / (time.time() - t0), 2)
            rc = 0
            return rc

        if args.from_state is not None:
            wait_frames(120)
            report["state_load"] = state_request("load", args.from_state)
            wait_frames(60)
            rc = gameplay(run, report)
        else:
            print("boot / logos (no input)")
            t0 = time.time()
            wait_frames(args.boot_frames)
            report["boot_fps"] = round(args.boot_frames / (time.time() - t0), 2)
            run.shot("before_title")
            rc = route_to_room(run, report)
            if rc == 0 and args.save_state is not None:
                report["state_save"] = state_request("save", args.save_state)
            if rc == 0:
                rc = gameplay(run, report)

        fps = report.get("gameplay_fps", 0)
        if rc == 0 and fps < args.min_fps:
            report["error"] = f"gameplay at {fps} FPS (< {args.min_fps})"
            rc = 1
        return rc
    except (GameExited, StateLoadFailed) as e:
        report["error"] = str(e)
        print(f"ABORTED: {e}")
        return 1
    finally:
        report["result"] = "ok" if rc == 0 else "fail"
        if "gameplay_stats" in report:
            report["summary"] = summary(report)
        report["wall_s"] = round(time.time() - t_start, 1)
        report["shots"] = run.shots
        report["still_running"] = proc.poll() is None
        if proc.poll() is None and not args.keep_open:
            try:
                send({"cmd": "quit"})
            except GameExited:
                pass
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=10)
                report["killed"] = True
        report["exit_code"] = proc.poll()
        if state_backup is not None:
            restore_mod_state(state_backup)
            report["hd_textures"] = [l.strip() for l in open(log.name, encoding="utf-8",
                                     errors="replace") if "texture pack" in l or "HD textures" in l]
        with open(os.path.join(out, "report.json"), "w", encoding="utf-8") as f:
            json.dump(report, f, indent=2)
        log.close()
        print(json.dumps({k: v for k, v in report.items()
                          if k not in ("shots", "gameplay_stats", "args", "cmdline", "checks",
                                       "state_load", "state_save")}, indent=2))
        print(f"session: {out}")


MOD_STATE = os.path.join(PCPORT, "mods", "state.toml")
HD_PACKAGE = "psx.enhancement.hd-textures"


def swap_mod_state(pack, dump):
    """Point the HD texture loader at PACK for this run. The runtime reads
    mods/state.toml next to the exe at boot; the original text comes back in
    restore_mod_state(). The package itself must be in mods/bundled."""
    if not os.path.isfile(os.path.join(PCPORT, "mods", "bundled", HD_PACKAGE, "1.0.0", "manifest.toml")):
        raise SystemExit(f"{HD_PACKAGE} is not in the run folder's mods/bundled")
    pack = os.path.abspath(pack)
    os.makedirs(os.path.join(pack, "replacements"), exist_ok=True)
    os.makedirs(os.path.join(pack, "dumps"), exist_ok=True)
    try:
        with open(MOD_STATE, encoding="utf-8") as f:
            backup = f.read()
    except FileNotFoundError:
        backup = ""
    with open(MOD_STATE, "w", encoding="utf-8", newline="\n") as f:
        f.write(f"""format_version = 2

[[package]]
id = "{HD_PACKAGE}"

[[feature]]
package_id = "{HD_PACKAGE}"
id = "hd-textures"
enabled = true
[feature.values]
dump = "{'true' if dump else 'false'}"
replacements = "true"
[feature.resources]
pack = "{pack.replace(os.sep, '/')}"
""")
    return backup


def restore_mod_state(backup):
    if backup:
        with open(MOD_STATE, "w", encoding="utf-8", newline="") as f:
            f.write(backup)
    elif os.path.exists(MOD_STATE):
        os.remove(MOD_STATE)


def in_room():
    """Jill takes input: the work struct is filled during the load text
    already, but its mode byte stays 0 until she is controllable."""
    p = player()
    return p is not None and p["mode"] != 0


def route_to_room(run, report, limit=2400):
    """Title (LOAD GAME is preselected when card 1 has a save) -> Memory card 1
    -> the first save on the card. The menu transitions vary in length, so
    this presses CROSS every 90 frames until Jill is controllable
    instead of trusting fixed waits. CROSS during the load text is harmless,
    and the presses keep the title from falling into the attract demo."""
    print("title -> load game -> card 1 -> file 1")
    start = frame()
    presses = 0
    while not in_room():
        if frame() - start > limit:
            check(report, "reached_room", False, f"not in a room after {limit} frames")
            run.shot("route_timeout")
            return 1
        run.press(CROSS, then=84)
        presses += 1
        if presses <= 12:
            run.shot(f"route_cross_{presses}")
    check(report, "reached_room", True, f"{presses} presses, {frame() - start} frames")
    return 0


def read_ram(addr, length):
    r = send({"cmd": "read_ram", "addr": f"0x{addr:08X}", "len": length})
    if not r.get("ok"):
        return None
    return bytes.fromhex(r["hex"])


def player():
    """Jill's work struct: +04 mode, +05 routine, +34/+3C X/Z (int32)."""
    b = read_ram(PLAYER, 0x70)
    if b is None:
        return None
    return {"mode": b[4], "routine": b[5],
            "x": struct.unpack_from("<i", b, 0x34)[0],
            "z": struct.unpack_from("<i", b, 0x3C)[0]}


def ui_flags():
    b = read_ram(UI_FLAGS, 4)
    return struct.unpack("<I", b)[0] if b else None


def check(report, name, ok, detail):
    report.setdefault("checks", []).append({"check": name, "ok": bool(ok), "detail": detail})
    print(f"  check {name}: {'OK' if ok else 'FAIL'} ({detail})")


def import_state(cards, session, slot):
    """Copy slot SLOT's state files from an earlier session into this one."""
    src = os.path.join(os.path.abspath(session), "memcards")
    files = glob.glob(os.path.join(src, "*", f"state_*_slot{slot:02d}.*"))
    if not files:
        raise SystemExit(f"no slot {slot} state under {src}")
    for f in files:
        dst = os.path.join(cards, os.path.relpath(f, src))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy2(f, dst)
    return files


if __name__ == "__main__":
    sys.exit(main())
