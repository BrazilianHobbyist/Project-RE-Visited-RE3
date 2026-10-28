#!/usr/bin/env python3
"""ws_playtest.py: playtest capture session for the widescreen investigation.

Run it next to the debug-tools build (tools/run_debug.ps1 starts both). Play
normally and press Ctrl+M in the game at each moment worth studying. The game
freezes its debug rings at that frame. This script notices the mark and saves:

  NNN_frame<F>/
    frame-00..03.json, .summary.json   GP0 draw lists of the last frames, each
                                       primitive with its guest function, OT
                                       rank, tpage, CLUT and UVs
    present.png                        the image shown on screen at frame F
    wide.png                           the runtime's own capture-mark image
    gpu_state.json                     display mode, draw area and ws state
    overlay.json                       overlay loader status
    ram.bin, scratchpad.bin            main RAM and scratchpad, read right
                                       after the mark (local only: game data)
    controls.csv                       the last minute, one row per frame:
                                       stick, the modern-controls decision
                                       (path, target, hold, camera cut),
                                       Jill's position/facing/routine and the
                                       camera, decoded from the frame history

Then it releases the mark so the next Ctrl+M works. To label the latest
capture, type a label in this terminal and press Enter (e.g. "subtitle",
"inventory"). Everything is indexed in index.csv in the session folder.

The session folder lives outside the repository (captures/ in the workspace),
because RAM dumps and frames contain game data and must never be committed or
packaged.
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import json
import os
import shutil
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
WORKSPACE = os.path.dirname(REPO)
sys.path.insert(0, os.path.join(REPO, "psxrecomp", "tools"))

from psx_gpu_frame import (  # noqa: E402
    DEFAULT_NATIVE_PORT, DebugConn, DebugError, capture, save_dump, save_summary,
)

try:
    import msvcrt  # Windows console: non-blocking label input
except ImportError:  # pragma: no cover
    msvcrt = None

INDEX_FIELDS = ["n", "folder", "mark_frame", "time", "frames_saved", "label", "errors"]

# Frame-history RAM snapshots (128 bytes each), recorded every frame by the
# runtime. Addresses are the ones the modern controls read (main.cpp,
# re3_screen_relative_move).
SNAPSHOTS = {
    0: 0x800CCBC4,  # player work: +04 mode, +05 routine, +34/+3C X/Z, +6E facing
    1: 0x80098990,  # camera view matrix (3x3 int16) + translation (int32 x3 @ +14)
    2: 0x800D4314,  # UI flags (0x00080808 = inventory / map / message)
}
CONTROL_SECONDS = 60
PATHS = {0: "none", 1: "free", 2: "forward_only", 3: "exit_inactive+tank",
         4: "exit_state+tank", 5: "exit_no_axis+tank", 6: "deadzone", 7: "dpad", 8: "inject_skip"}
CONTROL_FIELDS = [
    "frame", "pad_buttons", "path", "stick_x", "stick_y", "mode", "routine",
    "yaw_before", "yaw_after", "target_raw", "target", "hold", "cut", "injected",
    "fwd_x", "fwd_z", "cam_dist", "cam_sig", "player_x", "player_z", "facing",
    "ram_mode", "ram_routine", "cam_x", "cam_z", "ui_flags",
]


def decode_controls(src_csv, dst_csv):
    """Turn frame_history_dump rows into one readable row per frame."""
    import struct
    with open(src_csv, newline="", encoding="utf-8") as f, \
         open(dst_csv, "w", newline="", encoding="utf-8") as g:
        w = csv.DictWriter(g, fieldnames=CONTROL_FIELDS)
        w.writeheader()
        for r in csv.DictReader(f):
            gd = bytes.fromhex(r["game_data"])
            row = {"frame": r["frame"], "pad_buttons": r["pad_buttons"]}
            if gd[0] == ord("C"):
                (_, path, mode, routine, lx, ly, yb, ya, tr, tg, hold, cut, clear,
                 fx, fz, flen, sig) = struct.unpack("<BBBBhhhhhhBBHhhiI", gd)
                row.update(path=PATHS.get(path, path), stick_x=lx, stick_y=ly,
                           mode=mode, routine=routine, yaw_before=yb, yaw_after=ya,
                           target_raw=tr, target=tg, hold=hold, cut=cut,
                           injected=f"0x{clear:04X}", fwd_x=fx, fwd_z=fz,
                           cam_dist=flen, cam_sig=f"0x{sig:08X}")
            pl = bytes.fromhex(r["snap0"])
            row.update(ram_mode=pl[4], ram_routine=pl[5],
                       player_x=struct.unpack_from("<i", pl, 0x34)[0],
                       player_z=struct.unpack_from("<i", pl, 0x3C)[0],
                       facing=struct.unpack_from("<h", pl, 0x6E)[0] & 4095)
            cam = bytes.fromhex(r["snap1"])
            m = [v / 4096.0 for v in struct.unpack_from("<9h", cam, 0)]
            t = struct.unpack_from("<3i", cam, 0x14)
            row["cam_x"] = round(-(m[0] * t[0] + m[3] * t[1] + m[6] * t[2]))
            row["cam_z"] = round(-(m[2] * t[0] + m[5] * t[1] + m[8] * t[2]))
            row["ui_flags"] = f"0x{struct.unpack_from('<I', bytes.fromhex(r['snap2']), 0)[0]:08X}"
            w.writerow(row)


def write_json(path, obj):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(obj, f, indent=1)


def read_ram(conn, addr, length, path):
    r = conn.cmd("read_ram", addr=f"0x{addr:08X}", len=length)
    with open(path, "wb") as f:
        f.write(bytes.fromhex(r["hex"]))


class Session:
    def __init__(self, root, game_dir, frames, port):
        stamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
        self.dir = os.path.join(root, f"ws_{stamp}")
        os.makedirs(self.dir, exist_ok=True)
        self.game_dir = game_dir
        self.frames = frames
        self.conn = DebugConn("127.0.0.1", port, timeout=30.0)
        self.rows = []
        self.index = os.path.join(self.dir, "index.csv")
        self._save_index()

    def _save_index(self):
        with open(self.index, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=INDEX_FIELDS)
            w.writeheader()
            w.writerows(self.rows)

    def label_last(self, text):
        if not self.rows:
            print("  (no capture yet to label)")
            return
        self.rows[-1]["label"] = text
        self._save_index()
        print(f"  #{self.rows[-1]['n']} labelled: {text}")

    def grab(self, status):
        mark = int(status["mark_frame"])
        n = len(self.rows) + 1
        folder = f"{n:03d}_frame{mark}"
        out = os.path.join(self.dir, folder)
        os.makedirs(out, exist_ok=True)
        errors = []
        print(f"\n[#{n}] mark at frame {mark}: capturing...")

        def step(name, fn):
            try:
                fn()
            except (DebugError, OSError, KeyError, ValueError) as e:
                errors.append(f"{name}: {e}")
                print(f"  ! {name}: {e}")

        saved = 0
        # The mark lands on the frame being built; the newest complete frame
        # in the frozen ring is the one on screen when Ctrl+M was pressed.
        oldest, newest = (int(v) for v in status.get("gp0_ring", [0, mark]))
        base = min(mark, newest)
        # RE3 draws every other vblank (30 fps): skip frames with no primitives
        # and keep the newest self.frames that actually drew something.
        frame = base
        while saved < self.frames and frame >= oldest and frame > base - 4 * self.frames:
            tag = f"frame-{saved:02d}"
            drew = []

            def one(frame=frame, tag=tag):
                dump = capture(self.conn, frame=frame, count=65536, label=tag,
                               verify_ring=False)
                if not dump["prims"]:
                    return
                save_dump(dump, os.path.join(out, f"{tag}.json"))
                save_summary(dump, os.path.join(out, f"{tag}.summary.json"),
                             dump_name=f"{tag}.json")
                drew.append(frame)
            step(f"gpu_frame {frame}", one)
            if drew:
                saved += 1
            frame -= 1

        def present():
            ring = self.conn.cmd("present_image_ring_stats")
            if not ring.get("count"):
                raise DebugError("presented-image ring is empty")
            self.conn.cmd("present_image_ring_get", sequence=int(ring["newest_sequence"]),
                          path=os.path.join(out, "present.png"))
        step("present.png", present)
        step("gpu_state", lambda: write_json(
            os.path.join(out, "gpu_state.json"), self.conn.cmd("gpu_state")))
        step("overlay", lambda: write_json(
            os.path.join(out, "overlay.json"), self.conn.cmd("overlay_loader_status")))
        step("ram", lambda: read_ram(self.conn, 0x80000000, 0x200000,
                                     os.path.join(out, "ram.bin")))
        step("scratchpad", lambda: read_ram(self.conn, 0x1F800000, 0x400,
                                            os.path.join(out, "scratchpad.bin")))

        wide = status.get("wide_path") or ""
        if wide and status.get("wide_ok"):
            src = wide if os.path.isabs(wide) else os.path.join(self.game_dir, wide)
            if os.path.exists(src):
                step("wide.png", lambda: shutil.move(src, os.path.join(out, "wide.png")))

        def controls():
            raw = os.path.join(out, "frame_history.csv")
            self.conn.cmd("frame_history_dump", start=max(0, base - 60 * CONTROL_SECONDS),
                          end=base, out=raw)
            decode_controls(raw, os.path.join(out, "controls.csv"))
        step("controls", controls)

        step("release", lambda: self.conn.cmd("capture_mark", op="release"))

        self.rows.append({
            "n": n, "folder": folder, "mark_frame": base,
            "time": dt.datetime.now().strftime("%H:%M:%S"),
            "frames_saved": saved, "label": "", "errors": " | ".join(errors),
        })
        self._save_index()
        ok = "ok" if not errors else f"{len(errors)} error(s)"
        print(f"  -> {folder} ({saved} frame(s), {ok}). Type a label + Enter to name it.")


def wait_for_server(conn):
    print("Waiting for the game's debug server on port "
          f"{conn.port} (start the debug build)...")
    while True:
        try:
            ping = conn.cmd("ping")
            break
        except DebugError:
            time.sleep(1.0)
    # The runtime forgets snapshot regions on restart: set them every connect.
    for slot, addr in SNAPSHOTS.items():
        conn.cmd("set_snapshot", slot=slot, addr=f"0x{addr:08X}")
    return ping


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", type=int, default=DEFAULT_NATIVE_PORT)
    ap.add_argument("--frames", type=int, default=4,
                    help="draw lists to save per mark, walking back from it "
                         "(RE3 draws at 30 fps, so 4 = two game frames per buffer)")
    ap.add_argument("--out", default=os.path.join(WORKSPACE, "captures"))
    ap.add_argument("--game-dir", default=os.path.join(WORKSPACE, "ResidentEvil3-PCPort"),
                    help="game working directory (where the runtime writes its mark image)")
    args = ap.parse_args(argv)

    s = Session(args.out, args.game_dir, max(1, args.frames), args.port)
    ping = wait_for_server(s.conn)
    print(f"Connected (frame {ping.get('frame')}). Session: {s.dir}")
    print("In game: Ctrl+M = capture. Here: type a label + Enter for the last capture.")
    print("Ctrl+C here ends the session.\n")

    st = s.conn.raw("capture_mark", op="status")
    if st.get("frozen"):  # a mark left over from before this session
        s.conn.cmd("capture_mark", op="release")
    last_count = int(st.get("count", 0))
    typed = ""
    try:
        while True:
            try:
                st = s.conn.cmd("capture_mark", op="status")
                if st.get("frozen") and int(st.get("count", 0)) != last_count:
                    last_count = int(st["count"])
                    s.grab(st)
            except DebugError:
                print("\nDebug server gone (game closed?). Waiting for it to return...")
                wait_for_server(s.conn)
                last_count = int(s.conn.raw("capture_mark", op="status").get("count", 0))
            if msvcrt:
                while msvcrt.kbhit():
                    ch = msvcrt.getwche()
                    if ch in "\r\n":
                        print()
                        if typed.strip():
                            s.label_last(typed.strip())
                        typed = ""
                    elif ch == "\b":
                        typed = typed[:-1]
                    else:
                        typed += ch
            time.sleep(0.25)
    except KeyboardInterrupt:
        pass
    print(f"\nSession closed: {len(s.rows)} capture(s) in {s.dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
