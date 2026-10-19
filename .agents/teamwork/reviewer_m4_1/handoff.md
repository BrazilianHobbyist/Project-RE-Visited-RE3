# Handoff Report — Reviewer M4-1 (Milestone 4: Features 12 & 13)

## Review Summary

**Verdict**: **APPROVE**  
**Integrity Audit**: **0 VIOLATIONS** (No hardcoded test outputs, no facade logic, no shortcuts, no fabricated logs)  
**Overall Risk Assessment**: **LOW**

---

## 1. Observation

### 1.1 Source Code and Architecture Inspection
1. **`psxrecomp/runtime/include/re3_hd_ui.h`**:
   - Lines 12–31: Declares enum `re3_ui_screen_t` with 17 distinct screen states (`RE3_UI_SCREEN_NONE`, `RE3_UI_SCREEN_INVENTORY`, `RE3_UI_SCREEN_STATUS_FINE`, `RE3_UI_SCREEN_STATUS_CAUTION`, `RE3_UI_SCREEN_STATUS_DANGER`, `RE3_UI_SCREEN_STATUS_POISON`, `RE3_UI_SCREEN_MAP`, `RE3_UI_SCREEN_CLOCK`, `RE3_UI_SCREEN_SAVE`, `RE3_UI_SCREEN_EPILOGUE_0`..`7`, and sentinel `RE3_UI_SCREEN_COUNT`).
   - Lines 33–41: Defines `re3_ui_backdrop_info_t` struct with `has_hd`, `fallback_native_ui`, `gl_tex_id`, `width`, `height`, `file_path`, and `screen_name`.
   - Lines 43–49: Defines `re3_hd_ui_metrics_t` struct with hit/miss counters and active texture count.
   - Lines 51–157: Declares clean C interface (`extern "C"`) covering initialization, shutdown, status resolution, backdrop loading, blitting, fullscreen quad detection, memory screen detection, menu interception, sprite binding, metrics, and memory reader hook.

2. **`psxrecomp/runtime/src/re3_hd_ui.cpp`**:
   - Lines 75–76: Bounded LRU cache capacities configured as `kMaxCachedBackdrops = 32` and `kMaxCachedSprites = 256`.
   - Lines 126: Mutex `s_ui_mutex` protects all cache, map, and metric accesses.
   - Lines 245–273: `decode_png_to_rgba()` validates PNG signature (`89 50 4E 47 0D 0A 1A 0A`), verifies file bounds (8 bytes to 64 MB), and invokes `stbi_load_from_memory(..., 4)` to return authentic RGBA pixels.
   - Lines 302–315 & 370–376: Security path traversal protection rejects strings containing `..`, `/`, `\`, or `:` and validates canonical directory containment via `fs::weakly_canonical()`.
   - Lines 378–390: Boundary check on epilogue screens rejects indices `< 0` or `>= 8`.
   - Lines 471–481: `re3_hd_ui_resolve_status_backdrop()` maps condition byte `0x04` -> `status_fine`, `0x03` -> `status_caution`, `0x02` -> `status_danger`, `0x01` -> `status_poison`, and default/unknown -> `status_fine`.
   - Lines 535–551: Texture allocation uploads RGBA texels with `GL_CLAMP_TO_EDGE` and `GL_LINEAR`, with headless fallback generating deterministic handles.
   - Lines 608–660: `re3_hd_ui_render_backdrop()` binds target FBO (`s_hr_fbo`), binds texture unit, and renders a fullscreen triangle using `s_blit_program`.
   - Lines 662–669: `re3_hd_ui_is_fullscreen_quad()` verifies PS1 menu quad dimensions (`320x240`, `160x240`, `256x240`).
   - Lines 671–694: `re3_hd_ui_detect_screen_from_memory()` reads guest RAM at `0x000CCC97` (condition) and `0x000D1F3E` (epilogue count) via `read_guest_ram()`.
   - Lines 715–835: `re3_hd_ui_bind_sprite()` converts 64-bit `xxHash64` fingerprint to hex filename variants, checks missing cache, decodes PNG, generates mipmaps and sets anisotropic filtering up to 16x.

3. **`psxrecomp/runtime/src/gpu.c`**:
   - Line 55: Includes `"re3_hd_ui.h"`.
   - Lines 5174–5180 (inside `gp0_exec_textured_quad`):
     ```c
     if (g_re3_tex_config.hd_ui && re3_hd_ui_is_fullscreen_quad(x, y, w, h)) {
         uint32_t fbo = gl_renderer_get_hr_fbo();
         int scale = gl_renderer_get_hr_scale();
         if (re3_hd_ui_intercept_menu_quad(x, y, w, h, tpage, clut, fbo, 1024 * scale, 512 * scale)) {
             return;
         }
     }
     ```
   - Lines 5530–5536 (inside `gp0_exec_textured_rect`):
     ```c
     if (g_re3_tex_config.hd_ui && re3_hd_ui_is_fullscreen_quad(x0, y0, w, h)) {
         uint32_t fbo = gl_renderer_get_hr_fbo();
         int scale = gl_renderer_get_hr_scale();
         if (re3_hd_ui_intercept_menu_quad(x0, y0, w, h, current_texpage(), clut, fbo, 1024 * scale, 512 * scale)) {
             return;
         }
     }
     ```

4. **`psxrecomp/runtime/src/gpu_gl_renderer.c`**:
   - Line 84: Includes `"re3_hd_ui.h"`.
   - Lines 3786–3793:
     ```c
     if (cur_hd_tex == 0 && g_re3_tex_config.hd_ui) {
         uint16_t clut = (uint16_t)(((clut_y & 0x1FF) << 6) | ((clut_x >> 4) & 0x3F));
         uint64_t hash = re3_hd_tex_compute_from_vram(texpage, clut, lim, gpu_get_vram());
         if (hash != 0) {
             cur_hd_tex = re3_hd_ui_bind_sprite(hash);
             cur_is_hd = (cur_hd_tex != 0);
         }
     }
     ```
   - Lines 8586–8592: Implements `gl_renderer_get_hr_fbo()` and `gl_renderer_get_hr_scale()`.

5. **`psxrecomp/runtime/src/re3_textures.cpp`**:
   - Line 4: Includes `"re3_hd_ui.h"`.
   - Line 106: Calls `re3_hd_ui_init(s_pack_path);` during texture pack initialization.
   - Line 111: Calls `re3_hd_ui_shutdown();` during texture pack shutdown.

6. **`CMakeLists.txt`**:
   - Line 39: Target `RE3Recomp` compiles `"${PSXRECOMP_ROOT}/runtime/src/re3_hd_ui.cpp"`.

### 1.2 Independent Test Execution Commands and Results

1. **Compilation Command**:
   ```powershell
   cmake --build build
   ```
   **Result**: Exit Code 0 (Clean build, no errors, all targets up to date).

2. **Native C++ HD UI Unit Tests**:
   ```powershell
   .\build\test_re3_hd_ui_native.exe
   ```
   **Output**:
   ```
   ============================================
    Running Native re3_hd_ui (Milestone 4) Tests
   ============================================
   [TEST 1] test_lifecycle_and_activation... [PASS]
   [TEST 2] test_backdrop_resolution... [PASS]
   [TEST 3] test_status_ecg_mapping... [PASS]
   [TEST 4] test_backdrop_loading_and_fallback... [PASS]
   [TEST 5] test_ui_sprite_replacement... [PASS]
   [TEST 6] test_memory_detection_and_quad_interception... [PASS]
   [TEST 7] test_corrupt_png_resilience... [PASS]
   ============================================
    Results: 7 executed, 7 passed, 0 failed
   ============================================
   ```
   **Result**: Exit Code 0 (7/7 passed).

3. **Native C++ Adversarial Stress Tests**:
   ```powershell
   .\build\test_re3_m4_adversarial.exe
   ```
   **Output**:
   ```
   =====================================================
    Running Adversarial Stress Tests for Milestone 4 (HD UI)
   =====================================================
   [TEST 1] test_security_path_traversal... [PASS]
   [TEST 2] test_rapid_backdrop_switching_stress... [PASS]
   [TEST 3] test_sprite_cache_flooding_and_eviction... [PASS]
   [TEST 4] test_extreme_memory_states... [PASS]
   [TEST 5] test_reinit_stress... [PASS]
   =====================================================
    Adversarial Results: 5 executed, 5 passed, 0 failed
   =====================================================
   ```
   **Result**: Exit Code 0 (5/5 passed).

4. **Python E2E Test Suite**:
   ```powershell
   python tests/e2e/run_e2e_tests.py
   ```
   **Output**:
   ```
   ================================================================================
     RE3Recomp E2E Test Suite Execution - 155 Test Cases
   ================================================================================
   --------------------------------------------------------------------------------
     TEST EXECUTION SUMMARY
   --------------------------------------------------------------------------------
     Total Duration: 10.107 seconds
     Total Tests:    155
     Passed:         155
     Failed:         0
     Errors:         0
     Skipped:        0
   --------------------------------------------------------------------------------
     Breakdown by Tier:
       • TIER_1     : 85/85 passed [PASS]
       • TIER_2     : 35/35 passed [PASS]
       • TIER_3     : 15/15 passed [PASS]
       • TIER_4     : 5/5 passed [PASS]
       • TIER_5     : 15/15 passed [PASS]
   ================================================================================
   ```
   **Result**: Exit Code 0 (155/155 passed).

5. **Milestone Regression Verification**:
   - `.\build\test_re3_hd_textures_native.exe`: 5/5 passed.
   - `.\build\test_re3_hd_backgrounds_native.exe`: 5/5 passed.
   - `.\build\test_re3_m2_adversarial.exe`: 6/6 passed.
   - `.\build\test_re3_m3_adversarial.exe`: 8/8 passed.
   **Result**: 24/24 regression tests passed with zero regressions.

---

## 2. Logic Chain

1. **Integrity Audit**:
   - Source code in `re3_hd_ui.cpp`, `re3_hd_ui.h`, `gpu.c`, and `gpu_gl_renderer.c` was audited for cheating patterns.
   - No hardcoded test responses, dummy bypasses, or fabricated artifacts were present.
   - The implementation directly decodes real PNG images using `stbi_load_from_memory`, generates real OpenGL textures and shaders, handles real hardware memory addresses (`0x000CCC97`, `0x000D1F3E`), and intercepts actual geometry in the GPU command queue.

2. **Call-Graph Verification**:
   - Subsystem startup: `re3_textures_load_pack()` -> `re3_hd_ui_init()`.
   - Subsystem shutdown: `re3_textures_unload_pack()` / `re3_textures_shutdown()` -> `re3_hd_ui_shutdown()`.
   - Backdrop path resolution: `re3_hd_ui_resolve_backdrop_path()` -> canonical checks -> candidate filenames -> `fs::exists()`.
   - Backdrop blit: `gp0_exec_textured_quad()` / `gp0_exec_textured_rect()` -> `re3_hd_ui_is_fullscreen_quad()` -> `re3_hd_ui_intercept_menu_quad()` -> `re3_hd_ui_render_backdrop()` -> `s_hr_fbo`.
   - UI sprite replacement: `gl_renderer_draw_prims()` -> `re3_hd_ui_bind_sprite(hash)` -> `s_sprite_cache` / PNG decode -> `cur_hd_tex`.
   - Every declared API has an active callsite in the runtime pipeline.

3. **Memory Safety and Resource Bounding**:
   - The backdrop cache is strictly bounded to 32 entries and the sprite cache to 256 entries.
   - When the cache capacity is exceeded, least-recently-used (LRU) entries are removed and their OpenGL texture handles are released via `glDeleteTextures`.
   - The missing sprite cache stores 64-bit integer keys (`uint64_t`) in an unordered set, which clears on shutdown, preventing memory leaks during gameplay.

4. **Security and Resilience**:
   - Path traversal attempts (`..`, `\`, `/`, `:`) are caught and rejected prior to any filesystem operations.
   - Directory canonicalization prevents symlink or relative path escaping.
   - Epilogue gallery memory indicators are strictly bounded to indices 0..7.
   - Corrupt PNG files and non-PNG streams are rejected safely without crashing the host process.

---

## 3. Caveats

- **No Caveats**: All interface specifications, GPU hooks, memory mappings, and test cases function exactly as prescribed.
- **Informational Note**: `re3_hd_ui_detect_screen_from_memory()` uses an internal static buffer `static char ep_str[32];` for returning formatted epilogue names ("epilogue_0".."epilogue_7"). Because RE3Recomp processes GPU primitives deterministically on the main emulation thread, this is completely safe. For future code cleanliness, a constant string array or string_view could be used.

---

## 4. Conclusion

The Milestone 4 implementation (Features 12 & 13: HD User Interface & Menus Subsystem) is **approved without reservation**. It satisfies all functional requirements, security constraints, performance criteria, and test gates.

**Verdict: APPROVE**

---

## 5. Verification Method

To independently reproduce and verify this review from a clean shell:

1. **Build runtime and targets**:
   ```powershell
   cmake --build build
   ```
2. **Execute HD UI native unit tests**:
   ```powershell
   .\build\test_re3_hd_ui_native.exe
   ```
3. **Execute HD UI adversarial stress tests**:
   ```powershell
   .\build\test_re3_m4_adversarial.exe
   ```
4. **Execute complete Python E2E test suite (155 tests)**:
   ```powershell
   python tests/e2e/run_e2e_tests.py
   ```
5. **Execute regression suites**:
   ```powershell
   .\build\test_re3_hd_textures_native.exe
   .\build\test_re3_hd_backgrounds_native.exe
   .\build\test_re3_m2_adversarial.exe
   .\build\test_re3_m3_adversarial.exe
   ```
