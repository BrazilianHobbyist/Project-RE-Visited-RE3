# Dispatch: Explorer M4-1 (UI Screen Detection & Backdrop Resolution)

## Mission
Investigate 2D UI screen identification, menu quad interception, and HD backdrop image resolution for Milestone 4 (Feature 12).

## Authoritative Inputs
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- E2E Tests: `tests/e2e/` (specifically tests for Feature 12: HD UI 2D Screen Backdrops)
- Codebase paths:
  - `psxrecomp/runtime/src/gpu.c`
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`
  - `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`
  - `psxrecomp/runtime/src/re3_textures.cpp`

## Scope & Questions to Answer
1. How does the E2E test suite define and verify Feature 12 (`TestFeature12HdUiBackdrops` in `tests/e2e/test_tier1_features.py` or elsewhere)?
2. What are the exact screen names and candidate file paths in `mods/textures/<pack>/ui/` (e.g. `inventory.png`, `status.png`, `map.png`, `epilogue_*.png`, `clock.png`, uppercase/lowercase)?
3. How does RE3 signal active UI screens or menus in memory, or how does `gpu.c` / `gpu_gl_renderer.c` detect full-screen 2D UI quads?
4. How should the public interface `re3_hd_ui.h` be designed?
5. Write your complete findings to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_1\handoff.md`.

## 2026-10-18T07:09:17Z
You are Explorer M4-1 (UI Screen Detection & Backdrop Resolution).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_1\

Please read:
- ORIGINAL_REQUEST: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
- PROJECT: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- DISPATCH: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_1\DISPATCH.md
- E2E tests: tests/e2e/test_tier1_features.py (Feature 12)

Investigate UI screen detection, menu quad interception, candidate file paths in mods/textures/<pack>/ui/, and re3_hd_ui.h public interface design.
Write your complete report to c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_1\handoff.md.
Notify the orchestrator with send_message when done.
