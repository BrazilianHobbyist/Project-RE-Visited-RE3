# BRIEFING — 2026-10-18T05:25:00Z

## Mission
Investigate and design the Asynchronous Pre-fetcher & LRU Cache Architecture for Milestone 2 (HD Pre-rendered Backgrounds Engine) in RE3Recomp.

## 🔒 My Identity
- Archetype: explorer
- Roles: teamwork_preview_explorer
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_2\
- Original parent: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Milestone: Milestone 2 — HD Pre-rendered Backgrounds Engine

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Do NOT write source code in the codebase
- Write only to working directory (.agents/teamwork/explorer_m2_2/)
- Zero sycophancy, absolute technical honesty, exact line numbers and quotes

## Current Parent
- Conversation ID: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Updated: 2026-10-18T05:18:38Z

## Investigation State
- **Explored paths**:
  - `psxrecomp/runtime/src/hd_texture_pack.cpp` (lines 400-600, 1200-1329: mutex, cv, worker, decode pipeline, budget eviction)
  - `psxrecomp/runtime/src/psx_window_icon.cpp` (lines 18-21: STB_IMAGE_IMPLEMENTATION, STBI_NO_STDIO, STBI_ONLY_PNG)
  - `psxrecomp/runtime/third_party/stb_image.h` (lines 420-430: stbi_load_from_memory vs stbi_load with STBI_NO_STDIO)
  - `psxrecomp/runtime/src/gpu_gl_renderer.c` (lines 4608-4675: make_tex, s_hr_fbo, alloc_hr_targets; line 5142: SDL_GL_CreateContext; lines 8180-8250: room/cut tracking and present)
  - `psxrecomp/runtime/include/re3_textures.h` and `psxrecomp/runtime/src/re3_textures.cpp` (path resolution, pack config)
  - `tests/test_re3_textures_native.cpp` (test harness)
- **Key findings**:
  1. `stbi_load` is disabled project-wide via `STBI_NO_STDIO` in `psx_window_icon.cpp`; PNGs must be read into RAM and decoded using `stbi_load_from_memory`.
  2. Single OpenGL context (`s_ctx`) in `gpu_gl_renderer.c` requires all GL operations (`glGenTextures`, `glTexImage2D`, `glDeleteTextures`) to remain strictly on the main render thread. Background worker must perform only CPU file I/O and PNG decompression into staging RAM buffers.
  3. Room-level eager prefetching enqueues all discovered cuts of a room upon boundary change, placing the active cut at priority 0 and pruning obsolete requests from older rooms.
  4. 32-entry LRU cache (`std::list` + `std::unordered_map`) safely bounds VRAM (265MB at 1080p, 470MB at 1440p, 1GB at 4K UHD) while keeping adjacent rooms cached for instant door crossings.
  5. Fallback via `re3_hd_bg_has_hd_current()` seamlessly skips the HD blit if a texture is loading or absent, letting native PS1 320x240 VRAM rendering display with zero stutter.
- **Unexplored areas**: None for M2.2 scope.

## Key Decisions Made
- Confirmed single-worker thread model running at below-normal priority on Windows.
- Confirmed main-thread GL texture upload with immediate host staging RAM buffer release (`staging_pixels.reset()`).
- Established LRU capacity of 32 cuts and pre-indexed room cut catalog at initialization to eliminate disk probing during gameplay.

## Artifact Index
- DISPATCH.md — Task assignment and instructions
- BRIEFING.md — Persistent context index
- progress.md — Liveness heartbeat and progress log
- handoff.md — Final 5-component report
