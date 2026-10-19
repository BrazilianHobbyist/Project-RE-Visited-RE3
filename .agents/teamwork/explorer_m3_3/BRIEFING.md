# BRIEFING — 2026-10-18T06:33:40Z

## Mission
Investigate 3D actor polygon identification and tagging in gpu.c, UV normalization to [0.0, 1.0] with texel centering, transparent fallback behavior, and CMakeLists.txt build integration.

## 🔒 My Identity
- Archetype: explorer
- Roles: explorer, investigator, synthesizer
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_3
- Original parent: 7e5b962c-58f6-46df-a295-efcce3929cc7
- Milestone: Milestone 3

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Do NOT modify source code files outside of .agents/teamwork/explorer_m3_3/
- Adhere to Teamwork protocol (5-component handoff.md, progress.md heartbeat, BRIEFING.md maintenance)
- Absolute technical honesty and empirical evidence verification

## Current Parent
- Conversation ID: 7e5b962c-58f6-46df-a295-efcce3929cc7
- Updated: not yet

## Investigation State
- **Explored paths**:
  - `psxrecomp/runtime/src/gpu.c` (polygon execution GP0 0x24..0x3F, `is_3d_actor_polygon`, `gl_renderer_set_actor_tag`)
  - `psxrecomp/runtime/src/gpu_gl_renderer.c` (`TEX_VS`, `TEX_FS`, `gpu_textured_triangle`, batch key management, `s_actor_prim_tag`)
  - `psxrecomp/runtime/src/gpu_uv.h` (PS1 sampling model, center sampling, limits)
  - `psxrecomp/runtime/include/gpu_render.h` & `gpu_gl_renderer.h` (`gl_renderer_set_actor_tag` declarations)
  - `CMakeLists.txt` (`target_sources` with `re3_textures.cpp` and `re3_hd_backgrounds.cpp`)
  - `psxrecomp/runtime/src/re3_textures.cpp` & `re3_textures.h` (initialization and lifecycle)
  - `tests/e2e/` (155 passing tests across Tiers 1-5, UV normalization formulas in `texture_pipeline.py`)
- **Key findings**:
  1. `gpu.c` already tags actor polygons using `is_3d_actor_polygon(vx, vy, n)` in all 4 polygon commands (POLY_FT3, POLY_FT4, POLY_GT3, POLY_GT4), bracketing draws with `gl_renderer_set_actor_tag(1)` and `gl_renderer_set_actor_tag(0)`.
  2. `gpu_gl_renderer.c` receives this via `s_actor_prim_tag`. This enables exact per-polygon actor identification without modifying `gpu.c` tagging logic.
  3. UV normalization in `TEX_FS`: when `u_is_hd == 1`, mapping `hd_uv = (uv + vec2(0.5)) / 256.0` achieves perfect half-texel centering, matching test suite assertions and preventing border bleeding while seamlessly preserving perspective-correct hyperbolic interpolation (`v_uv_p`).
  4. Build integration requires adding `"${PSXRECOMP_ROOT}/runtime/src/re3_hd_textures.cpp"` to `target_sources(RE3Recomp PRIVATE ...)` in root `CMakeLists.txt`. Header-only `xxhash.h` needs no external link libraries.
  5. Subsystem lifecycle: `re3_textures_init` calls `re3_hd_tex_init(s_pack_path)` and `re3_textures_shutdown` calls `re3_hd_tex_shutdown()`, cleanly hooking into existing `main.cpp` calls.
  6. Edge cases and fallback:
     - Gouraud shading: `v_col.rgb * 2.0` modulation must be applied to HD texels when `v_raw == 0`.
     - Batch flushes: `s_tb_hd_tex` must be a batch key to ensure different textures flush before drawing.
     - Transparent fallback: when replacement is missing or disabled, returning `0` seamlessly samples `s_raw_tex` via `u_is_hd == 0`.
- **Unexplored areas**: None. All 6 questions answered with verified code evidence.

## Key Decisions Made
- Confirmed full design architecture and implementation specifications for M3-3 handoff.

## Artifact Index
- DISPATCH.md — Dispatch instructions
- BRIEFING.md — Working memory and state
- progress.md — Liveness heartbeat
- handoff.md — Comprehensive investigation and technical design report
