# BRIEFING — 2026-10-18T05:32:00Z

## Mission
Analyze OpenGL FBO composition, occlusion mask alignment, and build system integration for Milestone 2 (HD Pre-rendered Backgrounds Engine).

## 🔒 My Identity
- Archetype: explorer
- Roles: investigation, synthesis
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_3
- Original parent: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Milestone: Milestone 2 — HD Pre-rendered Backgrounds Engine

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Do NOT write source code (only metadata/reports in explorer_m2_3 folder)
- File workspace convention: write only to .agents/teamwork/explorer_m2_3/

## Current Parent
- Conversation ID: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Updated: 2026-10-18T05:18:38Z

## Investigation State
- **Explored paths**:
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`: lines 568 (`s_hr_tex`, `s_hr_fbo`, `s_hr_rb`), 4652-4675 (`alloc_hr_targets`), 1976-1993 (`hr_begin`), 3921-3950 (`gpu_fill`), 3955-4010 (`gpu_copy_rect`), 8157-8340 (`gl_renderer_present_vram`), 1558-1588 (`ACTOR_PRES_VS`), 1820-1856 (`BLIT_VS`, `BLIT_FS`), 7973-8045 (`present_target_quad`).
  - `psxrecomp/runtime/src/gpu.c`: lines 4688-4704 (`is_3d_actor_polygon`), 5080-5206 (`gp0_exec_textured_quad`), 5668-5697 (`gp0_exec_fill_rect`), 5730-5755 (`gp0_exec_draw_area_tl/br`).
  - `CMakeLists.txt` & `runtime.cmake`: target sources, `cxx_std_17`, includes.
  - `re3_textures.h` & `re3_textures.cpp`: path resolution, config struct.
  - `tests/e2e/`: Features 2, 3, 4, 5, 6 and scenarios 2 & 3.
- **Key findings**:
  - `s_hr_fbo` is the single authoritative high-resolution FBO (up to 4K / scale $S=8$).
  - Drawing the HD background quad in `s_hr_fbo` at backbuffer initialization (`gpu_fill` / `ClearImage` when $w=320, h=240$) places the backdrop before 3D actors.
  - RE3 foreground occlusion masks are `POLY_FT4` quads; re-mapping their UVs to screen-space coordinates $u=x/320.0, v=y/240.0$ and sampling the HD background texture guarantees 1:1 pixel parity over 3D actors.
  - Compositing both background and masks in `s_hr_fbo` natively supports 4:3 letterboxing, Pan & Scan (16:9 vertical tracking), and Stretch presentation paths.
  - Build integration requires adding `${PSXRECOMP_ROOT}/runtime/src/re3_hd_backgrounds.cpp` to `target_sources(RE3Recomp PRIVATE ...)`.
- **Unexplored areas**:
  - None within Milestone 2 scope.

## Key Decisions Made
- Confirmed hook point at `gpu_fill` during backbuffer clear.
- Validated mathematical formulas for screen-space UV remapping of foreground occlusion masks.
- Verified build integration configuration in `CMakeLists.txt`.

## Artifact Index
- DISPATCH.md — Dispatch log
- BRIEFING.md — Situational awareness
- progress.md — Liveness heartbeat
- handoff.md — Final investigation report
