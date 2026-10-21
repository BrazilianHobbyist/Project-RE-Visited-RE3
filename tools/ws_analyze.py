#!/usr/bin/env python3
"""ws_analyze.py: classify the draw lists of a ws_playtest session.

For every capture it reports, per frame, how primitives split by OT band
(front 0-15 = UI candidates, 16+ = scene) and by packet pool (the RAM block
the packet came from), and gives a verdict:

  scene       scene primitives only (exploration)
  scene+ui    scene with front-layer UI on top (messages, questions, pause)
  2d          front layers only, no scene (inventory, map, files, menus)
  empty       nothing drawn (black / FMV / loading)

    py -3 tools/ws_analyze.py <session folder> [--detail LABEL]

The split is the working hypothesis from docs/WIDESCREEN_FINDINGS.md; this
tool is how it gets checked against new captures.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
from collections import defaultdict

UI_MAX_OT = 15
DRAW_KINDS_SKIP = {"other", "env"}


def drawing(dump):
    return [p for p in dump["prims"] if p.get("verts") and p.get("kind") not in DRAW_KINDS_SKIP
            and not p["op_name"].startswith("copy_")]


def verdict(prims):
    ui = sum(1 for p in prims if p["ot"] <= UI_MAX_OT)
    scene = len(prims) - ui
    if not prims:
        return "empty", ui, scene
    if scene and ui:
        return "scene+ui", ui, scene
    return ("scene" if scene else "2d"), ui, scene


def bbox(ps):
    xs = [v[0] for p in ps for v in p["verts"]]
    ys = [v[1] for p in ps for v in p["verts"]]
    return (min(xs), min(ys), max(xs), max(ys)) if xs else None


def detail(prims):
    rows = defaultdict(list)
    for p in prims:
        band = "ui(0-15)" if p["ot"] <= UI_MAX_OT else "scene(16+)"
        pool = f"0x{int(p['src'], 16) >> 16:02X}xxxx"
        rows[(band, p["op_name"], pool)].append(p)
    for k in sorted(rows):
        v = rows[k]
        cl = sorted({str(p.get("clut")) for p in v})
        ots = sorted({p["ot"] for p in v})
        print(f"    {k[0]:<10} {k[1]:<14} pool {k[2]} n={len(v):>4} ot={ots[0]}..{ots[-1]} "
              f"bbox={bbox(v)} cluts={cl[:5]}{'...' if len(cl) > 5 else ''}")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("session")
    ap.add_argument("--detail", action="append", default=[],
                    help="label (or folder) to print the per-group breakdown for")
    args = ap.parse_args(argv)

    with open(os.path.join(args.session, "index.csv"), newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        folder = os.path.join(args.session, r["folder"])
        verdicts = []
        for i in range(8):
            path = os.path.join(folder, f"frame-{i:02d}.json")
            if not os.path.exists(path):
                break
            with open(path, encoding="utf-8") as f:
                prims = drawing(json.load(f))
            verdicts.append(verdict(prims))
        names = sorted({v[0] for v in verdicts})
        counts = " ".join(f"{v[1]}/{v[2]}" for v in verdicts)
        print(f"{r['n']:>3} {r['label'] or '-':<28} {'/'.join(names):<10} ui/scene per frame: {counts}")
        if r["label"] in args.detail or r["folder"] in args.detail:
            with open(os.path.join(folder, "frame-00.json"), encoding="utf-8") as f:
                detail(drawing(json.load(f)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
