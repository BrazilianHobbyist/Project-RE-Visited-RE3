# Forensic Audit Report: Milestone 2 — HD Pre-rendered Backgrounds Engine

**Work Product**: Milestone 2 implementation (`re3_hd_backgrounds.h`, `re3_hd_backgrounds.cpp`, `gpu_gl_renderer.c`, `CMakeLists.txt`, `tests/test_re3_hd_backgrounds_native.cpp`)
**Integrity Mode**: DEMO MODE (read directly from `ORIGINAL_REQUEST.md`)
**Verdict**: **CLEAN**

---

### Phase Results
- **Check 1 (Dynamic computation vs hardcoded results)**: PASS — `re3_hd_bg_resolve_file`, `re3_hd_bg_has_hd_current`, `re3_hd_bg_render_current`, and `re3_hd_bg_get_screen_uv` compute all values dynamically from real filesystem lookups, guest memory states, and geometry dimensions.
- **Check 2 (Genuine decoding & OpenGL texture management)**: PASS — Authentic PNG loading and decompression via `stbi_load_from_memory`, OpenGL 2D texture allocation (`glGenTextures`, `glTexImage2D`), LRU cache management (32 entries max), and blit drawing via GLSL shader into `s_hr_fbo`.
- **Check 3 (No fabricated verification outputs / Independent execution)**: PASS — `build.ps1`, native test harness (`test_re3_hd_backgrounds_native.exe`), stress test harness (`test_re3_textures_stress.exe`), and Python E2E suite (85/85 Tier 1, 155/155 all tiers) were compiled and executed independently by the auditor with zero failures.
- **Check 4 (End-to-end call graph verification)**: PASS — Full call chain verified from `gpu_fill`, `actor_interp_commit_new_frame`, and `gl_renderer_present_vram` -> `psx_read_byte(0x000D1F08/9/A)` -> `re3_hd_bg_on_room_change` -> worker thread decode -> GL upload -> `s_hr_fbo` draw.
- **Check 5 (Regression and silent omission check)**: PASS — Git diff analysis verified that no existing functionality was disabled, commented out, or bypassed.

---

## 1. Observation

### 1.1 Source Code and Implementation Verifications
- **File Resolution (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp:378-471`)**:
  `re3_hd_bg_resolve_file` generates candidate strings dynamically:
  ```cpp
  std::snprintf(cand, sizeof(cand), "R%1X%02X_%02X.png", stage, room, cut);
  std::snprintf(cand, sizeof(cand), "R%1X%02X_%u.png", stage, room, cut);
  std::snprintf(cand, sizeof(cand), "r%1x%02x_%02x.png", stage, room, cut);
  std::snprintf(cand, sizeof(cand), "r%1x%02x_%u.png", stage, room, cut);
  ```
  Path traversal is rejected via `base_pack.find("..")` and `fs::weakly_canonical`. No hardcoded stage/room constants exist.
- **Decompression and Memory Release (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp:236-267, 323-368`)**:
  Decompression executes on a background thread (`worker_thread_func`) via `stbi_load_from_memory`. Texture upload happens on the main thread via `glTexImage2D`. Immediately following upload, `entry.staging_pixels.reset()` is called, freeing host CPU memory.
- **Occlusion Mask UVs (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp:720-726`)**:
  Screen-space UV mapping dynamically divides screen coordinates by native resolution:
  ```cpp
  out_uv[0] = static_cast<float>(x) / 320.0f;
  out_uv[1] = static_cast<float>(y) / 240.0f;
  out_uv[2] = static_cast<float>(x + w) / 320.0f;
  out_uv[3] = static_cast<float>(y + h) / 240.0f;
  ```
- **Call-Graph Integrations in `gpu_gl_renderer.c`**:
  - Line 809 in `actor_interp_commit_new_frame`:
    ```c
    extern uint8_t psx_read_byte(uint32_t addr);
    uint8_t cur_stage = psx_read_byte(0x000D1F08);
    uint8_t cur_room  = psx_read_byte(0x000D1F09);
    uint8_t cur_cut   = psx_read_byte(0x000D1F0A);
    re3_hd_bg_on_room_change(cur_stage, cur_room, cur_cut);
    ```
  - Lines 3930-3957 in `gpu_fill`:
    ```c
    extern uint8_t psx_read_byte(uint32_t addr);
    uint8_t cur_stage = psx_read_byte(0x000D1F08);
    uint8_t cur_room  = psx_read_byte(0x000D1F09);
    uint8_t cur_cut   = psx_read_byte(0x000D1F0A);
    re3_hd_bg_on_room_change(cur_stage, cur_room, cur_cut);
    ...
    if (!gpu_display_is_depth24() && (w == 320 && h == 240) && re3_hd_bg_has_hd_current()) {
        glViewport(x * s_hr_scale, y * s_hr_scale, w * s_hr_scale, h * s_hr_scale);
        glDisable(GL_SCISSOR_TEST);
        re3_hd_bg_render_current(s_hr_fbo, w * s_hr_scale, h * s_hr_scale);
        glViewport(0, 0, VRAM_W * s_hr_scale, VRAM_H * s_hr_scale);
    }
    ```
  - Line 8204 in `gl_renderer_present_vram`:
    ```c
    extern uint8_t psx_read_byte(uint32_t addr);
    uint8_t cur_stage = psx_read_byte(0x000D1F08);
    uint8_t cur_room  = psx_read_byte(0x000D1F09);
    uint8_t cur_cut   = psx_read_byte(0x000D1F0A);
    re3_hd_bg_on_room_change(cur_stage, cur_room, cur_cut);
    ```

### 1.2 Auditor Independent Execution Outputs

1. **Clean Build of Runtime (`powershell -ExecutionPolicy Bypass -File "build.ps1"`)**:
   ```
   ============================================
    RE3Recomp Build Script
    Config: Release
   ============================================
   [CONFIG]  Configurando CMake...
   -- psxrecomp: psx_libchdr from vendored archive third_party/libchdr-6cde5348eb118da3baf94f75a69577a005a484fd.tar.gz
   -- psxrecomp: using prebuilt/system SDL3 (skip FetchContent)
   -- psxrecomp: SDL backend = SDL3
   -- psxrecomp: PSX_NETPLAY=OFF — netplay TUs compile as stubs (no recomp-net)
   -- psxrecomp: local rewind disabled (PSX_REWIND=OFF)
   -- BIOS backends linked: OpenBIOS;SCPH1001
   -- psxrecomp game_version: 0.0.0 (from VERSION=0.0.0)
   -- psxrecomp: linking generated game C (full runtime) — C:/Users/victo/Documents/Projetos/Decomp - SOTC/RE3Recomp/generated/SLUS_009.23_dispatch.c
   -- psxrecomp RE3Recomp: APP_ICON=C:/Users/victo/Documents/Projetos/Decomp - SOTC/RE3Recomp/assets/psxrecomp.ico (RC=C:/Users/victo/AppData/Local/Programs/retcomm-toolchain/bin/windres.exe)
   -- psxrecomp: ZLIB static (PSX_STATIC_RUNTIME)
   -- psxrecomp RE3Recomp: PSX_MAX_PLAYERS=1
   -- Vulkan backend: PSX_ENABLE_VULKAN=ON but SDK headers/glslc not found - gpu_vk_renderer.c builds as a software stub
   -- Configuring done (0.7s)
   -- Generating done (0.1s)
   -- Build files have been written to: C:/Users/victo/Documents/Projetos/Decomp - SOTC/RE3Recomp/build
   [BUILD] Compilando...
   [0/2] Re-checking globbed directories...
   [1/7] Verifying recompiled game C exists for RE3Recomp
   [2/7] Building RC object CMakeFiles/RE3Recomp_app_icon.dir/RE3Recomp_app_icon.rc.obj
   [3/7] Staging mod catalog for RE3Recomp (5 package(s) -> mods/bundled)
   [4/7] Verifying staged mod catalog for RE3Recomp
   -- [RE3Recomp] staged mod catalog OK: 5 package(s) = 0 game-owned + 5 framework-owned, all under mods/bundled, no mods/packages
   [5/7] Building C object CMakeFiles/RE3Recomp.dir/psx_bios_registry.c.obj
   [6/7] Linking CXX executable Resident_Evil_3__Nemesis_Recompiled.exe
   [OK] Build concluído!
      Executável: C:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\build\Resident_Evil_3__Nemesis_Recompiled.exe
   ```
   Exit code: 0.

2. **Native Unit/Integration Test Suite (`.\build\test_re3_hd_backgrounds_native.exe`)**:
   ```
   ============================================
    Running Native re3_hd_backgrounds Tests
   ============================================
   [TEST 1] test_file_resolution...
     [PASS] test_file_resolution
   [TEST 2] test_screen_space_uv...
     [PASS] test_screen_space_uv
   [TEST 3] test_async_prefetch_and_cache...
   psxrecomp: [Textures] Active pack: "SeamlessHD" at "C:/Users/victo/AppData/Local/Temp/re3_m2_native_6334_26500/mods/textures/SeamlessHD" [BG:ON, Models:ON, UI:ON, Dump:OFF]
   psxrecomp: [Textures] Subsystem shutdown complete.
     [PASS] test_async_prefetch_and_cache
   [TEST 4] test_corrupt_png_resilience...
   psxrecomp: [Textures] Active pack: "CorruptPack" at "C:/Users/victo/AppData/Local/Temp/re3_m2_native_19169_15724/mods/textures/CorruptPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
   psxrecomp: [Textures] Subsystem shutdown complete.
     [PASS] test_corrupt_png_resilience
   [TEST 5] test_transparent_fallback_and_lifecycle...
     [PASS] test_transparent_fallback_and_lifecycle
   ============================================
    Results: 5 executed, 5 passed, 0 failed
   ============================================
   ```
   Exit code: 0.

3. **Native Stress Test Harness (`.\build\test_re3_textures_stress.exe`)**:
   ```
   ====================================================
   Results: 7 executed, 7 passed, 0 failed
   ====================================================
   ```
   Exit code: 0.

4. **Python E2E Test Suite (Tier 1 & Full Suite)**:
   - Command: `python tests/e2e/run_e2e_tests.py --tier 1`
     - Duration: 3.452 seconds
     - Tests: 85/85 passed, 0 failed, 0 errors, 0 skipped.
   - Command: `python tests/e2e/run_e2e_tests.py`
     - Duration: 9.577 seconds
     - Breakdown:
       - TIER_1: 85/85 passed
       - TIER_2: 35/35 passed
       - TIER_3: 15/15 passed
       - TIER_4: 5/5 passed
       - TIER_5: 15/15 passed
     - Total: 155/155 passed, 0 failed, 0 errors, 0 skipped.
   Exit code: 0.

---

## 2. Logic Chain

1. From Observation 1.1, `re3_hd_bg_resolve_file` calculates file candidates directly from the integer parameters `stage`, `room`, and `cut` via `snprintf` format specifiers, and checks actual filesystem existence using `std::filesystem::exists`. No hardcoded strings or test bypasses exist.
2. From Observation 1.1, image decompression genuinely invokes `stbi_load_from_memory` on a background worker thread with `THREAD_PRIORITY_BELOW_NORMAL`. Texture creation and uploading strictly invoke OpenGL driver APIs (`glGenTextures`, `glTexImage2D`) on the thread holding the render context. Staging memory is immediately reclaimed (`entry.staging_pixels.reset()`), and LRU eviction manages VRAM resident capacity within 32 textures.
3. From Observation 1.1, the call graph cleanly binds into the PS1 GPU rendering loop: `gpu_fill` intercepts standard PS1 320x240 screen clears and blits the 4K texture into `s_hr_fbo` using a fullscreen triangle shader. When an HD background is not present or pending decode, `re3_hd_bg_has_hd_current()` returns false, retaining native PS1 VRAM presentation.
4. From Observation 1.1 and 1.2, git diff analysis confirms zero regressions, zero silent deletions, and zero disabled features.
5. From Observation 1.2, all test harnesses, compilers, and validation suites were run independently by this auditor, producing 100% passing results without errors or warnings.

---

## 3. Caveats

- In headless unit test environments where no OpenGL context is created (`SDL_GL_GetCurrentContext() == nullptr`), `re3_hd_bg_render_current` safely returns without issuing OpenGL draw commands. This behavior is deliberate and required for headless testing and CI runners.
- No caveats regarding implementation integrity.

---

## 4. Conclusion

The Milestone 2 work product (HD Pre-rendered Backgrounds Engine) implements genuine, robust, and performant logic. It contains no hardcoded shortcuts, facade implementations, or fabricated outputs. All forensic checks pass without exception.

**Binary Verdict**: **CLEAN**.

---

## 5. Verification Method

To independently reproduce this audit:

1. **Clean build RE3Recomp**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "build.ps1"
   ```
2. **Compile and run native backgrounds test harness**:
   ```powershell
   c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_hd_backgrounds_native.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_hd_backgrounds_native.exe
   .\build\test_re3_hd_backgrounds_native.exe
   ```
3. **Execute Python E2E verification suites**:
   ```powershell
   python tests/e2e/run_e2e_tests.py --tier 1
   python tests/e2e/run_e2e_tests.py
   ```
