# Dispatch: Worker M3-1 (Milestone 3 Implementation)

## Mission
Implement Milestone 3: 3D Model & Actor HD Texture Subsystem in RE3Recomp under Level 2 engineering governance.

## Mandatory Reading
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- Explorer M3-1 Handoff: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_1\handoff.md`
- Explorer M3-2 Handoff: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_2\handoff.md`
- Explorer M3-3 Handoff: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_3\handoff.md`

## Mandatory Integrity Warning
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

## Level 2 Rigor Instructions
- Pre-Flight Baseline: Check `git status` / `git diff` before editing.
- End-to-end Call-graph: Ensure all functions (`re3_hd_tex_init`, `re3_hd_tex_shutdown`, `re3_hd_tex_compute_hash`, `re3_hd_tex_bind_replacement`, `re3_hd_tex_dump_if_enabled`) are connected from initialization through renderer draw calls.
- Zero silent omission: Retain all existing functionality, including M1 & M2 features, PS1 VRAM fallback, and all existing batching/blending logic.

## Scope of File Ownership
You have exclusive write ownership over:
- `psxrecomp/runtime/include/xxhash.h`
- `psxrecomp/runtime/include/re3_hd_textures.h`
- `psxrecomp/runtime/src/re3_hd_textures.cpp`
- `psxrecomp/runtime/src/gpu_gl_renderer.c` (HD texture unit 2 binding, TEX_FS shader branching, batch key `s_tb_hd_tex`, actor HD lookup)
- `psxrecomp/runtime/src/re3_textures.cpp` (lifecycle integration: init & shutdown)
- `CMakeLists.txt` (add `re3_hd_textures.cpp` to `RE3Recomp` sources)
- `tests/test_re3_hd_textures_native.cpp` (native unit test for hash & replacement verification)

## Tasks
1. Create `psxrecomp/runtime/include/xxhash.h` with single-header `XXH_INLINE_ALL` xxHash64 implementation.
2. Create `psxrecomp/runtime/include/re3_hd_textures.h` matching the agreed interface contracts.
3. Create `psxrecomp/runtime/src/re3_hd_textures.cpp`:
   - Two-tier cache: positive map (`unordered_map<uint64_t, uint32_t>`) and negative set (`unordered_set<uint64_t>`).
   - Replacement file resolution in `mods/textures/<pack>/replacements/` (`%016llx`, `%016llX`, `%llx`, `%llX`, `.png`, `.PNG`).
   - PNG decoding via `stbi_load_from_memory` (DO NOT define `STB_IMAGE_IMPLEMENTATION` here).
   - OpenGL upload: trilinear mipmaps (`glGenerateMipmap` / `GL_LINEAR_MIPMAP_LINEAR`), 16x anisotropic filtering (`GL_TEXTURE_MAX_ANISOTROPY_EXT`), clamp to edge.
   - Clean shutdown: release all GL textures via `glDeleteTextures`.
   - `re3_hd_tex_compute_hash` using 16-byte packed header `<HHHHHHI`, palette words (zero-padded if short), and raw texels.
   - `re3_hd_tex_dump_if_enabled` placeholder / basic export for M5 tooling.
4. Integrate in `gpu_gl_renderer.c`:
   - Declare uniforms `u_hd_tex` and `u_is_hd`. Query locations in raster init.
   - In `TEX_FS`: add `u_hd_tex` (sampler2D) and `u_is_hd` (int). When `u_is_hd != 0`, calculate `hd_uv = (uv + vec2(0.5)) / 256.0`, sample `u_hd_tex`, discard if alpha < 0.01, apply Gouraud lighting `clamp(rgb * v_col.rgb * 2.0, 0.0, 1.0)` when `v_raw == 0`.
   - In `gpu_textured_triangle`: when `(s_actor_prim_tag && g_re3_tex_config.hd_models)`, compute/lookup replacement texture. Add `s_tb_hd_tex` to batch key checks so batch flushes if HD texture changes.
   - In `flush_tex_batch`: bind texture unit 2 (`PSXGL_TEXTURE0 + 2`), set `u_hd_tex = 2`, `u_is_hd = 1` if active HD texture, else `u_is_hd = 0`.
5. Integrate in `re3_textures.cpp`:
   - Call `re3_hd_tex_init(s_pack_path)` in `re3_textures_init`.
   - Call `re3_hd_tex_shutdown()` in `re3_textures_shutdown`.
6. Update `CMakeLists.txt` to add `re3_hd_textures.cpp`.
7. Build and Test:
   - Run build script: `powershell -ExecutionPolicy Bypass -File .\build.ps1`
   - Run E2E test suite: `python tests/e2e/run_e2e_tests.py`
   - Ensure all 155 tests continue to pass.
   - Create a native test runner/executable or script validating M3 specifically.

Write your complete handoff report to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m3_1\handoff.md`.
Notify the orchestrator with send_message upon completion.

## 2026-10-18T06:37:41Z
Received dispatch message: Implement Milestone 3 (3D Model & Actor HD Texture Subsystem) under Level 2 engineering governance.

