# Review & Adversarial Challenge Report: Reviewer M2 2
**Milestone**: Milestone 2 — HD Pre-rendered Backgrounds Engine  
**Target Subsystem**: `re3_hd_backgrounds` and `gpu_gl_renderer.c` OpenGL Integration  
**Verdict**: **APPROVE**  
**Overall Risk Assessment**: **LOW**

---

## 1. Observation

1. **OpenGL Pipeline Integration in `gpu_gl_renderer.c`**:
   - `actor_interp_commit_new_frame` (lines 805–809):
     ```c
     extern uint8_t psx_read_byte(uint32_t addr);
     uint8_t cur_stage = psx_read_byte(0x000D1F08);
     uint8_t cur_room  = psx_read_byte(0x000D1F09);
     uint8_t cur_cut   = psx_read_byte(0x000D1F0A);
     re3_hd_bg_on_room_change(cur_stage, cur_room, cur_cut);
     ```
     Accurately reads guest memory for Stage (`0x000D1F08`), Room (`0x000D1F09`), and Cut (`0x000D1F0A`) upon committing vertex interpolation frames.
   - `gpu_fill` (lines 3930–3934):
     ```c
     extern uint8_t psx_read_byte(uint32_t addr);
     uint8_t cur_stage = psx_read_byte(0x000D1F08);
     uint8_t cur_room  = psx_read_byte(0x000D1F09);
     uint8_t cur_cut   = psx_read_byte(0x000D1F0A);
     re3_hd_bg_on_room_change(cur_stage, cur_room, cur_cut);
     ```
     Invokes room change latching prior to clearing the backbuffer.
   - `gpu_fill` hook (lines 3952–3957):
     ```c
     if (!gpu_display_is_depth24() && (w == 320 && h == 240) && re3_hd_bg_has_hd_current()) {
         glViewport(x * s_hr_scale, y * s_hr_scale, w * s_hr_scale, h * s_hr_scale);
         glDisable(GL_SCISSOR_TEST);
         re3_hd_bg_render_current(s_hr_fbo, w * s_hr_scale, h * s_hr_scale);
         glViewport(0, 0, VRAM_W * s_hr_scale, VRAM_H * s_hr_scale);
     }
     ```
     Verifies: (1) not in 24-bit movie mode (`!gpu_display_is_depth24()`), (2) target size is full screen (`w == 320 && h == 240`), and (3) an HD background is decoded and ready (`re3_hd_bg_has_hd_current()`). Sets scaled viewport (`x * s_hr_scale, y * s_hr_scale, w * s_hr_scale, h * s_hr_scale`), disables scissor test, blits to `s_hr_fbo`, and restores standard VRAM viewport.
   - `gl_renderer_present_vram` (lines 8200–8204):
     ```c
     extern uint8_t psx_read_byte(uint32_t addr);
     uint8_t cur_stage = psx_read_byte(0x000D1F08);
     uint8_t cur_room  = psx_read_byte(0x000D1F09);
     uint8_t cur_cut   = psx_read_byte(0x000D1F0A);
     re3_hd_bg_on_room_change(cur_stage, cur_room, cur_cut);
     ```
     Latches camera state at the presentation boundary before buffer swap.

2. **Screen-Space UV Calculation for Occlusion Masks**:
   - In `psxrecomp/runtime/src/re3_hd_backgrounds.cpp` (lines 720–726):
     ```cpp
     void re3_hd_bg_get_screen_uv(int x, int y, int w, int h, float out_uv[4]) {
         if (!out_uv) return;
         out_uv[0] = static_cast<float>(x) / 320.0f;
         out_uv[1] = static_cast<float>(y) / 240.0f;
         out_uv[2] = static_cast<float>(x + w) / 320.0f;
         out_uv[3] = static_cast<float>(y + h) / 240.0f;
     }
     ```
     Formulas evaluated: `u0 = x / 320.0f`, `v0 = y / 240.0f`, `u1 = (x + w) / 320.0f`, `v1 = (y + h) / 240.0f`.
     Null pointer guard prevents dereference exceptions.

3. **Fallback Semantics & Resilience**:
   - In `re3_hd_backgrounds.cpp`:
     - If texture is missing or still decoding, `re3_hd_bg_has_hd_current()` returns `false`, causing `gpu_fill` to bypass `re3_hd_bg_render_current` and fall back to the native PS1 VRAM rendering pipeline.
     - If `g_re3_tex_config.hd_backgrounds` is `false`, `re3_hd_bg_has_hd_current()` immediately returns `false`.
     - In `decode_png_file` (lines 241–254), strict validation of PNG magic bytes (`0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A`) and size bounds (`8 < size <= 64 MB`) rejects corrupt payloads safely without crashing.
     - `stbi_load_from_memory` operates purely on host buffers; staging RAM is immediately freed (`staging_pixels.reset()`) following texture upload to GL.
     - Texture uploads and LRU cache eviction (capped at 32 resident textures) occur exclusively on the OpenGL render thread, adhering to OpenGL context single-thread execution rules.

4. **Security & Path Traversal Guards**:
   - `re3_hd_bg_resolve_file` enforces:
     - `base_pack.find("..") != std::string::npos` check rejects directory traversal.
     - `fs::weakly_canonical` validates that canonical background path prefix strictly matches canonical pack path prefix.

5. **Independent Build and Test Execution Outputs**:
   - **Runtime Build**: `powershell -ExecutionPolicy Bypass -File "build.ps1"`
     - Exit code: 0
     - Binary produced: `C:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\build\Resident_Evil_3__Nemesis_Recompiled.exe`
   - **Native Unit Test Suite**: `.\build\test_re3_hd_backgrounds_native.exe`
     - Tests executed: 5
     - Tests passed: 5 (`test_file_resolution`, `test_screen_space_uv`, `test_async_prefetch_and_cache`, `test_corrupt_png_resilience`, `test_transparent_fallback_and_lifecycle`)
     - Tests failed: 0
     - Exit code: 0
   - **Native Stress Harness**: `.\build\test_re3_textures_stress.exe`
     - Tests executed: 7
     - Tests passed: 7
     - Exit code: 0
   - **Full E2E Suite**: `python tests/e2e/run_e2e_tests.py`
     - Total duration: 9.726 seconds
     - Total tests: 155
     - Passed: 155
     - Failed: 0
     - Errors: 0
     - Breakdown: Tier 1 (85/85), Tier 2 (35/35), Tier 3 (15/15), Tier 4 (5/5), Tier 5 (15/15)

---

## 2. Logic Chain

1. From Observation 1, `psx_read_byte` is consistently queried at `0x000D1F08`, `0x000D1F09`, and `0x000D1F0A` across all three critical stages: frame interpolation commit, VRAM fill clear, and display presentation. This guarantees synchronization between game logic camera cuts and high-definition background prefetching.
2. From Observation 1, the hook in `gpu_fill` specifically gates the HD blit behind `!gpu_display_is_depth24()`, preventing background rendering over 24-bit FMV video playback, and checks `(w == 320 && h == 240)` to prevent interference with smaller 2D clears or UI boxes.
3. From Observation 1 and 2, the viewport calculations and `re3_hd_bg_get_screen_uv` math align 1:1 with 320x240 screen coordinates, scaling cleanly to any FBO resolution without sub-pixel seam or texture distortion.
4. From Observation 3 and 4, the architecture decouples file I/O and PNG decoding (background thread) from OpenGL resource allocation (render thread). This prevents frame stutters while eliminating OpenGL cross-thread context violation risks.
5. From Observation 3, fallback is transparent and zero-overhead: any cache miss or disabled state leaves the native PS1 VRAM buffer intact.
6. From Observation 5, all verification commands compiled and executed cleanly, achieving 100% pass rates across unit, stress, and end-to-end tiers without integrity shortcuts or stubbing.

---

## 3. Caveats

1. The OpenGL blit pipeline in `re3_hd_bg_render_current` assumes modern OpenGL 3.3 core profile shaders (gl_VertexID triangle generation). System environments lacking OpenGL 3.3 support fall back cleanly due to extension loading checks (`load_gl_extensions`).
2. No caveats regarding implementation bugs or integrity compromises.

---

## 4. Conclusion

Milestone 2 (HD Pre-rendered Backgrounds Engine) implements genuine, robust, thread-safe logic fulfilling all requirements of `ORIGINAL_REQUEST.md` (§R1) and architectural specifications of `PROJECT.md`. No integrity violations, hardcoded shortcuts, or facade implementations were detected. The verdict is **APPROVE**.

---

## 5. Verification Method

To independently reproduce verification:

1. **Compile the runtime executable**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "build.ps1"
   ```
   *Expected outcome*: Exit code 0, linking `Resident_Evil_3__Nemesis_Recompiled.exe`.

2. **Execute native unit tests**:
   ```powershell
   .\build\test_re3_hd_backgrounds_native.exe
   ```
   *Expected outcome*: `Results: 5 executed, 5 passed, 0 failed` with exit code 0.

3. **Execute native stress harness**:
   ```powershell
   .\build\test_re3_textures_stress.exe
   ```
   *Expected outcome*: `Results: 7 executed, 7 passed, 0 failed` with exit code 0.

4. **Execute complete Python E2E suite**:
   ```powershell
   python tests/e2e/run_e2e_tests.py
   ```
   *Expected outcome*: `155/155 passed [PASS]` across all 5 tiers with exit code 0.

---

## 6. Review Summary & Findings

- **Verdict**: **APPROVE**
- **Integrity Check**: **PASS** (Zero integrity violations; genuine dynamic logic and call-graph verification confirmed)
- **Verified Claims**:
  - `psx_read_byte` calls at `0x000D1F08`, `0x000D1F09`, `0x000D1F0A` in `actor_interp_commit_new_frame`, `gpu_fill`, and `gl_renderer_present_vram` -> VERIFIED.
  - `gpu_fill` hook with viewport setting, scissor disabling, depth24 check, and FBO blit -> VERIFIED.
  - Screen-space UV formulas `u0 = x / 320.0f`, `v0 = y / 240.0f`, `u1 = (x + w) / 320.0f`, `v1 = (y + h) / 240.0f` -> VERIFIED.
  - Transparent fallback on missing, disabled, or corrupt files -> VERIFIED.
  - LRU texture cache eviction and zero steady-state CPU RAM overhead -> VERIFIED.
- **Coverage Gaps**: None.
- **Unverified Items**: None.
