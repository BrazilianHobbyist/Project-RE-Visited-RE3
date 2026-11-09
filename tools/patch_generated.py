#!/usr/bin/env python3
"""patch_generated.py: the RE3 edits to the recompiled C in generated/.

Fast doors and the door audio fade change four instructions of the game and
add two calls. The edits are found by what the recompiler writes for each
instruction (its MIPS address and word), never by line number, so they apply
to any regeneration of the same disc, on the maintainer's machine or the
player's. Each edit must match exactly once; an edit already present is
skipped, so running this twice changes nothing.

  py -3 tools\\patch_generated.py [generated_dir]      apply (build.ps1 runs this)
  py -3 tools\\patch_generated.py --check [dir]        report only; exit 1 if any
                                                       edit neither applies nor is present
"""

from __future__ import annotations

import glob
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_DIR = os.path.join(os.path.dirname(HERE), "generated")

# (name, original text, edited text). Lines are joined with the file's own
# line ending.
EDITS = [
    ("fast doors: skip the door wait (0x80016160)",
     ["    if (_bc_80016160) {"],
     ["    extern int g_fast_doors;",
      "    if (_bc_80016160 || g_fast_doors) {"]),
    ("fast doors: door fade step 16 -> 5, door audio end (0x80016190)",
     ["    { uint32_t _pgx1 = cpu->gpr[0]; cpu->gpr[2] = 16;",
      "    PGXP_ALU(0x24020010u, cpu->gpr[2], _pgx1, 0x00000010u); }  /* 0x80016190: 0x24020010 */"],
     ["    extern int g_fast_doors;",
      "    { extern void re3_door_audio_end(void); re3_door_audio_end(); }",
      "    { uint32_t _pgx1 = cpu->gpr[0]; cpu->gpr[2] = g_fast_doors ? 5 : 16;",
      "    PGXP_ALU(0x24020010u, cpu->gpr[2], _pgx1, (uint32_t)cpu->gpr[2]); }  /* 0x80016190: 0x24020010 */"]),
    ("fast doors: fade step 8 -> 4 (0x8006FEF0)",
     ["    { uint32_t _pgx1 = cpu->gpr[0]; cpu->gpr[2] = 8;",
      "    PGXP_ALU(0x24020008u, cpu->gpr[2], _pgx1, 0x00000008u); }  /* 0x8006FEF0: 0x24020008 */"],
     ["    extern int g_fast_doors;",
      "    { uint32_t _pgx1 = cpu->gpr[0]; cpu->gpr[2] = g_fast_doors ? 4 : 8;",
      "    PGXP_ALU(0x24020008u, cpu->gpr[2], _pgx1, (uint32_t)cpu->gpr[2]); }  /* 0x8006FEF0: 0x24020008 */"]),
    ("fast doors: fade step 8 -> 4 (0x8006FF8C)",
     ["    { uint32_t _pgx1 = cpu->gpr[0]; cpu->gpr[2] = 8;",
      "    PGXP_ALU(0x24020008u, cpu->gpr[2], _pgx1, 0x00000008u); }  /* 0x8006FF8C: 0x24020008 */"],
     ["    extern int g_fast_doors;",
      "    { uint32_t _pgx1 = cpu->gpr[0]; cpu->gpr[2] = g_fast_doors ? 4 : 8;",
      "    PGXP_ALU(0x24020008u, cpu->gpr[2], _pgx1, (uint32_t)cpu->gpr[2]); }  /* 0x8006FF8C: 0x24020008 */"]),
    ("door audio begin (entry of 0x80015498)",
     ["    debug_server_log_call_entry(0x80015498u);"],
     ["    debug_server_log_call_entry(0x80015498u);",
      "    { extern void re3_door_audio_begin(void); re3_door_audio_begin(); }"]),
]


def main() -> int:
    args = [a for a in sys.argv[1:] if a != "--check"]
    check = "--check" in sys.argv
    gdir = args[0] if args else DEFAULT_DIR
    files = sorted(glob.glob(os.path.join(gdir, "*.c")))
    if not files:
        print(f"[X] no generated C in {gdir}")
        return 1
    texts = {f: open(f, "rb").read().decode("utf-8") for f in files}
    changed = set()
    bad = 0
    for name, old, new in EDITS:
        hits_old, hits_new = [], []
        for f, t in texts.items():
            nl = "\r\n" if "\r\n" in t else "\n"
            o, n = nl.join(old) + nl, nl.join(new) + nl
            hits_old += [(f, o, n)] * t.count(o)
            if t.count(n):
                hits_new.append(f)
        if hits_new:   # (an insertion's original is part of its edit)
            print(f"[=] {name}: already applied")
            continue
        if len(hits_old) != 1:
            print(f"[X] {name}: original found {len(hits_old)} times (expected 1)")
            bad += 1
            continue
        f, o, n = hits_old[0]
        print(f"[+] {name}: {os.path.basename(f)}" + (" (check only)" if check else ""))
        if not check:
            texts[f] = texts[f].replace(o, n, 1)
            changed.add(f)
    for f in changed:
        open(f, "wb").write(texts[f].encode("utf-8"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
