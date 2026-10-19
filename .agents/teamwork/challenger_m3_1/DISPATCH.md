# Dispatch: Challenger M3-1 (xxHash64 Adversarial & Empirical Verification)

## Mission
Empirically stress-test and adversarially verify the xxHash64 fingerprinting implementation in Milestone 3.

## Authoritative Inputs
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- Worker M3-1 Handoff: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m3_1\handoff.md`

## Adversarial Verification Tasks
1. Write and execute an adversarial test harness (in C++ or Python) that stresses:
   - Extreme boundary UV coordinates: `(0, 0, 0, 0)`, `(255, 255, 255, 255)`, inverted limits `(255, 255, 0, 0)`, single-pixel boxes.
   - Depth modes: 4bpp (`depth=0`, 16 palette entries), 8bpp (`depth=1`, 256 palette entries), 15bpp direct (`depth=2`, 0 palette entries), invalid depth values (e.g. 3).
   - Palette zero-padding: short palettes (e.g. 2 words) zero-padded to 16/256 words vs exact 16/256 words.
   - VRAM row wrapping: coordinates near VRAM boundaries (x >= 1024, y >= 512) to verify circular buffer handling without memory corruption.
   - Collision resistance: generate 10,000 distinct texture payloads and verify zero collisions across xxHash64 digests.
2. Confirm bit-exact parity between C++ `re3_hd_tex_compute_hash` and Python `compute_texture_xxhash64`.
3. State your empirical verdict clearly as **CONFIRMED** or **DEFECTS_FOUND** in `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m3_1\handoff.md`.

## 2026-10-18T06:57:06Z
Received orchestrator activation:
You are Challenger M3-1 (xxHash64 Adversarial Testing).
Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m3_1\
Stress-test xxHash64 fingerprinting with boundary conditions, depth modes (4bpp/8bpp/15bpp), palette padding, VRAM row wrapping, and collision testing.
Write complete report to c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m3_1\handoff.md with verdict: CONFIRMED or DEFECTS_FOUND.
Notify orchestrator via send_message when done.
