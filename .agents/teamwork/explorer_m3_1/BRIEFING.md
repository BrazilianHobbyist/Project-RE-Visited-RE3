# BRIEFING — 2026-10-18T06:36:00Z

## Mission
Investigate xxHash64 fingerprinting and texture payload extraction for 3D model & actor HD textures in RE3Recomp (Milestone 3).

## 🔒 My Identity
- Archetype: explorer
- Roles: investigation, analysis, synthesis
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_1
- Original parent: 7e5b962c-58f6-46df-a295-efcce3929cc7
- Milestone: M3 (3D Model & Actor HD Texture Subsystem)

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Analyze gpu.c, gpu_gl_renderer.c, vram.c, and existing texture infrastructure
- Propose exact hashing algorithm, data buffer composition, xxhash.h integration, and C/C++ function signatures
- Deliver findings in handoff.md following 5-component handoff report

## Current Parent
- Conversation ID: 7e5b962c-58f6-46df-a295-efcce3929cc7
- Updated: 2026-10-18T06:36:00Z

## Investigation State
- **Explored paths**:
  - `psxrecomp/runtime/src/gpu.c`: GP0 commands 0x24-0x27, 0x2C-0x2F, 0x34-0x37, 0x3C-0x3F; `is_3d_actor_polygon`, `gl_renderer_set_actor_tag`.
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`: `gpu_textured_triangle`, `flush_tex_batch`, `TEX_FS` shader, `s_actor_prim_tag`.
  - `psxrecomp/runtime/src/gpu_uv.h`: `psx_uv_tri_limits`, `psx_uv_tri_world_limits`, `psx_uv_tri_center_sample`.
  - `psxrecomp/runtime/src/hd_texture_pack.cpp`: CRC32 VRAM upload tracking and CLUT hashing reference.
  - `tests/e2e/harness/texture_pipeline.py`: Reference Python implementation for `compute_texture_xxhash64`, UV normalization, and aniso filtering.
  - `tests/e2e/harness/xxhash64.py`: Canonical 64-bit xxHash implementation and constants.
  - `tests/e2e/test_tier1_features.py` & `test_tier2_boundaries.py`: Boundary and feature test assertions for xxHash64.
- **Key findings**:
  - Composite payload: 16-byte header `[tpage:u16, clut:u16, u_min:u16, v_min:u16, u_max:u16, v_max:u16, flags:u32]`, followed by palette (32B for 4bpp, 512B for 8bpp, 0B for 15bpp), followed by raw VRAM texels.
  - Hashing algorithm: official xxHash64 with `seed = 0`. Produces 16-character lowercase hex string `{hash:016x}.png`.
  - Zero-copy streaming via `XXH64_update` allows hashing directly from VRAM scanlines and palette entries without intermediate memory allocation.
  - `is_3d_actor_polygon` in `gpu.c` already tags actor primitives with `gl_renderer_set_actor_tag(1)` during rendering.
- **Unexplored areas**: None. All questions in dispatch fully resolved.

## Key Decisions Made
- Design `re3_hd_tex_compute_hash` with streaming xxHash64 to support both pre-extracted pixel buffers and direct VRAM streaming.
- Place `xxhash.h` in `psxrecomp/runtime/include/xxhash.h` with `XXH_INLINE_ALL` support for zero-dependency integration.
- Standardize filename format to lowercase 16-character hex (`%016llx.png`).

## Artifact Index
- DISPATCH.md — Task assignment and requirements
- BRIEFING.md — Working memory and status
- progress.md — Liveness heartbeat
- handoff.md — Final investigation report
