# Overlays compiled from the disc (AOT)

`overlays.json` is a `psxrecomp AOT methods v1` profile for Resident Evil 3
(SLUS-00923). It lets the upstream pipeline (`psxrecomp/tools/aot_overlay_pipeline.py`)
rebuild the native overlay cache from the original disc alone, instead of only
from the code a tester happened to run while playing.

It holds facts only: file names, offsets, sizes, addresses and SHA-256 hashes.
No game bytes. `tools/make_aot_profile.py` regenerates it from a disc, and
`enemy_modules.json` lists which rooms carry each enemy module.

## What is code on the disc

Found by reading the disc and the executable, then checked against the runtime
captures (those are a cross-check only, never an input).

**Menu overlays, `CD_DATA/BIN/*.BIN` (17 files).** Each file is loaded whole at one of
three fixed addresses:
- `0x80194000`: DIEDEMO, ENDING, TITLE.
- `0x801C2000`: EPILOG, GEARBOX, JILL_SEL, LTSOUT, MEM_CARD, MUSICBOX, OPENING,
  OPTION, PC_SYS, R214_OL, RESULT, SELECT, STAFF_R.
- `0x80184000`: WARNING.

The file loader at `0x80032214` picks `0x80100000` or `0x80194000` from a flag
in the file table. The pipeline pins those instruction words as `words` checks.
For each file, the address is the only one where its internal calls land on
function prologues. TITLE was a near tie between `0x80194000` and
`0x80192800`; the loader word and the captures settle it on `0x80194000`.

**Enemy code modules, inside the room files `CD_DATA/STAGE*/R*.BIN` (155 rooms).**
A room file has a header `{total size, member count, {size, tag} x count}`,
followed by its members aligned to 2 KB sectors. Members whose tag ends in
`0001` are code linked for slot A, `0x80100000`. Members ending in `0002` are
the same module linked for slot B, `0x8010D000`.

- There are 21 distinct modules, so 42 images.
- Every internal call of every module lands on a prologue in its own slot and
  never in the other slot. The generator asserts this.
- The other members are textures (`..0100`), models (`..0000`) and room data.

Enemy modules declare their size rounded down to a whole word. The room header
stores exact byte sizes (for example `0xCA2D`), and the tail is data. The C++
jump-table resolver rejects every table of a producer whose end is not
word-aligned, while the Python verifier does not. That mismatch is worth
reporting upstream.

## Framework change: a third bounded-switch form

Most `switch` statements in the enemy modules have this shape:

```
sltiu; beq; X; lui; addiu; sll; addu; lw; nop; jr
```

X is one unrelated instruction in the bounds branch's delay slot. Upstream
recognised only `sltiu; beq; nop; sll` and `sltiu; beq; lui; addiu; sll`.

Counts across the title's images:
- 141 sites use this form;
- 137 sites use the form upstream already handled;
- 7 of the new-form sites are in the main executable.

The recogniser now accepts it in both copies, which stay in parity:
- `recompiler/src/function_analysis.cpp`, tested in `reachable_discovery_test.cpp`;
- `tools/compile_overlays.py`, tested in `test_aot_overlay_discovery.py`.

X must not be control flow and must not write the index, table or upper
registers. Every other proof is unchanged. This changes the codegen hash, so the
cache tag became `cg18_a8839e97_gcc15c8ed0_f2`. Regenerating the game C changed
only those 7 executable switches, which became native jump tables.

The PSXRecomp update to `ffbd9d1c` (2026-10-28) changed the code generator
again, so the tag is now `cg18_a601f18f_gcc15c8ed0_f2`. The pipeline was rerun
from the disc: 59 of 59 pairs, all valid, 2952 manifest rows.

## Results and limits

`expected_records = 59` (17 + 42) makes the pipeline stop when the inventory
changes. The `release` audit publishes 59 of 59 pairs, all with valid guards,
2952 manifest rows.

**Coverage against what was actually played.** For every instruction in the
71 runtime capture files, I checked whether it is byte-identical to a disc image
and inside a compiled disc function:

| Region | Distinct executed instructions | Covered |
|---|---|---|
| Enemy modules | 14810 | 100% |
| Menus | 6266 | 100% |

The other 25 (kernel RAM, and a copy inside the EXE's own range) are out of
scope.

**Disc-only run.** The attract demo was run with only this cache and
`PSX_OVERLAY_AUTOCOMPILE_OFF=1`, using a debug-tools build and
`dirty_ram_stats`. Interpreted instructions:

- Enemy modules: 0. Before the switch fix it was about 60k.
- Menus: about 1.9k, from one TITLE switch whose table base is defined far
  from its use. That is a fourth form, not handled.
- Kernel RAM and the EXE-range copy: about 0.5M. These were never overlay
  code, and the count is the same with the played cache.

An image inventory is not proof of complete execution coverage. Runtime
autocompile and the interpreter stay enabled as the fallback.

Two other room files, `R70F` and `R71A` in STAGE7, were flagged by the generic
detector as whole-file code at odd addresses. That is wrong for a container, so
every room file is in `generic_exclude_files`, and room code comes only from
the declared members.
