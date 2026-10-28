#!/usr/bin/env python3
"""make_font_upscale.py: sharp font from the player's own texture dump.

Reads the font atlases the HD texture loader dumped from the player's copy of
the game, scales each 4x with EPX/Scale2x applied twice (a pixel-art scaler:
diagonals get smooth steps, flat areas stay flat, no blur) and writes them as
replacements under the same names. The result is derived from the game's art:
it only ever lives in the player's run folder and must never be committed or
packaged.

RE3 font atlases (measured 2026-10-27, docs in PROGRESS.md):
  564BA4ED51F23272  messages, item names, file text (one image per palette:
                    white for messages, blue for the inventory, ...)
  A054F93CA3CFD0A5  GAME CONFIG / KEY CONFIG menu font

Each palette is a separate replacement (the loader keys on texture + palette
hash). The largest dumped rectangle of each pair wins; glyphs drawn outside it
keep the original pixels.

  py -3 tools\\make_font_upscale.py --pack <pack folder>
"""

from __future__ import annotations

import argparse
import glob
import os

from PIL import Image

FONT_SOURCES = ("564BA4ED51F23272", "A054F93CA3CFD0A5")
SCALE = 4


def epx2(img):
    """Scale2x/EPX: each texel becomes 2x2, copying a neighbour only where two
    neighbours agree, so diagonals stay crisp and flat areas stay flat."""
    w, h = img.size
    src = img.load()
    out = Image.new("RGBA", (w * 2, h * 2))
    dst = out.load()
    for y in range(h):
        for x in range(w):
            p = src[x, y]
            a = src[x, y - 1] if y > 0 else p
            b = src[x + 1, y] if x < w - 1 else p
            c = src[x - 1, y] if x > 0 else p
            d = src[x, y + 1] if y < h - 1 else p
            dst[2 * x, 2 * y] = a if (c == a and c != d and a != b) else p
            dst[2 * x + 1, 2 * y] = b if (a == b and a != c and b != d) else p
            dst[2 * x, 2 * y + 1] = c if (d == c and d != b and c != a) else p
            dst[2 * x + 1, 2 * y + 1] = d if (b == d and b != a and d != c) else p
    return out


def font_dumps(dump_dir):
    """{(source, palette): path} with the largest dumped rectangle of each.
    Name: texupload-P4-<source>-<palette>-<WxH words>-<x>-<y>-<w>x<h>-P<a>-<b>.png"""
    best = {}
    for path in glob.glob(os.path.join(dump_dir, "texupload-P4-*.png")):
        parts = os.path.basename(path)[:-4].split("-")
        if len(parts) != 10 or parts[2] not in FONT_SOURCES:
            continue
        w, h = (int(v) for v in parts[7].split("x"))
        key = (parts[2], parts[3])
        if key not in best or w * h > best[key][1]:
            best[key] = (path, w * h)
    return {k: v[0] for k, v in best.items()}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--pack", required=True, help="texture pack folder (dumps/ in, replacements/ out)")
    args = ap.parse_args()

    found = font_dumps(os.path.join(args.pack, "dumps"))
    if not found:
        raise SystemExit("no font atlas in the dump: play with Dump textures on until a message shows")
    out = os.path.join(args.pack, "replacements")
    os.makedirs(out, exist_ok=True)
    for (source, palette), path in sorted(found.items()):
        img = epx2(epx2(Image.open(path).convert("RGBA")))
        img.save(os.path.join(out, os.path.basename(path)))
        print(f"{source} {palette}: {os.path.basename(path)} -> {img.size[0]}x{img.size[1]}")


if __name__ == "__main__":
    main()
