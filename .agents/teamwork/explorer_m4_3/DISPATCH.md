# Dispatch: Explorer M4-3 (HD UI Sprite & Icon Replacements)

## Mission
Investigate Feature 13: HD UI Sprite & Icon Replacements, hash-based sprite replacement, and E2E test requirements for Milestone 4.

## Authoritative Inputs
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- E2E Tests: `tests/e2e/` (specifically tests for Feature 13: HD UI Sprite & Icon Replacements)
- Codebase paths:
  - `psxrecomp/runtime/src/re3_hd_textures.cpp`
  - `psxrecomp/runtime/include/re3_hd_textures.h`
  - `psxrecomp/runtime/src/gpu.c`
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`

## Scope & Questions to Answer
1. How does the E2E test suite define and verify Feature 13 (`TestFeature13HdUiSprites` in `tests/e2e/test_tier1_features.py` or elsewhere)?
2. How are individual UI sprites (inventory items, weapon icons, health condition bar, fonts) identified? Does it use xxHash64 computed from sprite texels/CLUT/tpage?
3. Where are sprite replacement PNGs placed in the texture pack (e.g., `mods/textures/<pack>/ui/sprites/<hash>.png` or `replacements/` or `ui/`)?
4. How should the pipeline handle sprite transparency and alpha cutouts?
5. Write your complete findings to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_3\handoff.md`.

## 2026-10-18T07:09:17Z
You are Explorer M4-3 (HD UI Sprite & Icon Replacements).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_3\

Please read:
- ORIGINAL_REQUEST: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
- PROJECT: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- DISPATCH: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_3\DISPATCH.md
- E2E tests: tests/e2e/test_tier1_features.py (Feature 13)

Investigate Feature 13: UI sprite and icon replacements, hash-based identification (xxHash64), replacement path structure in mods/textures/<pack>/ui/sprites/, and alpha cutout handling.
Write your complete report to c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_3\handoff.md.
Notify the orchestrator with send_message when done.
