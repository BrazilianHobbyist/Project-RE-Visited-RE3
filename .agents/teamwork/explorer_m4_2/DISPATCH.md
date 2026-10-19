# Dispatch: Explorer M4-2 (UI Blitting, FBO Integration & Dynamic Overlays)

## Mission
Investigate rendering HD UI backdrops into `s_hr_fbo` and preserving interactive dynamic text, item sprites, cursors, and ECG health animations for Milestone 4 (Feature 12).

## Authoritative Inputs
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- Codebase paths:
  - `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`
  - `psxrecomp/runtime/src/re3_textures.cpp`

## Scope & Questions to Answer
1. How does `re3_hd_backgrounds.cpp` currently blit into `s_hr_fbo` at the beginning of frames (`re3_hd_bg_render_current`)?
2. Can `re3_hd_ui` reuse or share the blitting shader (`s_bg_prog`) or FBO blit pipeline in `gpu_gl_renderer.c`?
3. How are dynamic UI overlays (text, inventory items, selection cursors, health condition monitor) layered on top of the HD backdrop without being occluded or distorted?
4. How should asynchronous loading / caching of UI backdrops be managed (positive/negative cache, preloading UI screens on init)?
5. Write your complete findings to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_2\handoff.md`.


## 2026-10-18T07:09:17Z
[Message] timestamp=2026-10-18T07:09:17Z sender=7e5b962c-58f6-46df-a295-efcce3929cc7 priority=MESSAGE_PRIORITY_HIGH content=You are Explorer M4-2 (UI Blitting, FBO Integration & Dynamic Overlays).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_2\

Please read:
- ORIGINAL_REQUEST: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
- PROJECT: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- DISPATCH: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_2\DISPATCH.md

Investigate blitting HD UI backdrops into s_hr_fbo, preserving interactive dynamic text, item sprites, cursors, and ECG health animations on top.
Analyze re3_hd_backgrounds.cpp and gpu_gl_renderer.c.
Write your complete report to c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_2\handoff.md.
Notify the orchestrator with send_message when done.
