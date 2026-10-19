# BRIEFING — 2026-10-18T06:34:00Z

## Mission
Investigate HD texture loading, replacement caching, trilinear/anisotropic filtering, and OpenGL pipeline integration (TEX_FS, u_is_hd, texture unit 2) for Milestone 3.

## 🔒 My Identity
- Archetype: explorer
- Roles: Read-only investigation: analyze problems, synthesize findings, produce structured reports.
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_2\
- Original parent: 7e5b962c-58f6-46df-a295-efcce3929cc7
- Milestone: M3 (3D Model & Actor HD Texture Subsystem)

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Analyze replacement loading, texture caching, OpenGL shader branching (u_is_hd), texture unit 2 binding, trilinear mipmaps, and 16x anisotropic filtering.
- Produce comprehensive handoff.md report.

## Current Parent
- Conversation ID: 7e5b962c-58f6-46df-a295-efcce3929cc7
- Updated: not yet

## Investigation State
- **Explored paths**:
  - `psxrecomp/runtime/src/gpu_gl_renderer.c` (TEX_VS, TEX_FS, build_program_ex, s_tex_prog, s_tb batching, flush_tex_batch, s_actor_prim_tag)
  - `psxrecomp/runtime/src/re3_hd_backgrounds.cpp` & `re3_hd_backgrounds.h` (GL extension loader, stb_image decoding, candidate matching)
  - `psxrecomp/runtime/src/re3_textures.cpp` & `re3_textures.h` (configuration, pack resolution, path normalization, lifecycle)
  - `psxrecomp/runtime/src/gpu_uv.h` & `gpu_render.h` (PS1 UV conventions, triangle dispatch)
  - `CMakeLists.txt` (target_sources for RE3Recomp)
  - `psxrecomp/runtime/src/psx_window_icon.cpp` (STB_IMAGE_IMPLEMENTATION single definition authority)
- **Key findings**:
  - `gpu_gl_renderer.c` uses GLSL 330 with dual-source output (`frag`, `blend_factor`). `TEX_FS` handles textured prims.
  - Texture unit 0 (`PSXGL_TEXTURE0`) binds VRAM mirror; Unit 1 (`PSXGL_TEXTURE0 + 1`) binds palette. Unit 2 (`PSXGL_TEXTURE0 + 2` / `GL_TEXTURE2`) is completely free for HD replacements!
  - `u_is_hd` uniform switches `TEX_FS` between PS1 VRAM sampling and HD texture sampling on unit 2.
  - HD sampling in `TEX_FS`: `vec2 hd_uv = (uv + 0.5) * (1.0 / 256.0);` with half-texel centering, `GL_CLAMP_TO_EDGE`.
  - Batching in `gpu_gl_renderer.c`: `s_tb_hd_tex` acts as a batch key, splitting batches whenever HD replacement changes.
  - Positive cache (`unordered_map<uint64_t, GLuint>`) + negative cache (`unordered_set<uint64_t>`) ensures O(1) lookups and zero frame hitching.
  - Anisotropic filtering up to 16x (`GL_TEXTURE_MAX_ANISOTROPY_EXT`) and trilinear mipmaps (`glGenerateMipmap` + `GL_LINEAR_MIPMAP_LINEAR`) configured at upload.
  - `stb_image.h` must NOT define `STB_IMAGE_IMPLEMENTATION` in `re3_hd_textures.cpp` (already defined in `psx_window_icon.cpp`).
  - Transparent fallback: when `u_is_hd == 0`, pipeline runs 100% vanilla PS1 code with zero side effects.
- **Unexplored areas**: none within M3-2 scope. All questions answered.

## Key Decisions Made
- Fully designed `re3_hd_textures.h`, `re3_hd_textures.cpp`, and `gpu_gl_renderer.c` integration.
- Formulated exact GLSL code, C struct and function contracts, candidate file resolution, and lifecycle hooks.

## Artifact Index
- handoff.md — Final investigation and recommendations report (in preparation)
