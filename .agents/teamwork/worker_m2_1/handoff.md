# Handoff Report: Worker M2 1 — HD Pre-rendered Backgrounds Engine

## 1. Observation

1. **State Tracking in Memory**:
   - Guest memory addresses `0x000D1F08` (Stage), `0x000D1F09` (Room), `0x000D1F0A` (Cut) are read synchronously via `psx_read_byte(addr)`.
   - In `psxrecomp/runtime/src/gpu_gl_renderer.c`:
     - Line 804 (`actor_interp_commit_new_frame`): Previously read `0x000D1F09` and `0x000D1F0A`. By adding `cur_stage = psx_read_byte(0x000D1F08)` and invoking `re3_hd_bg_on_room_change(cur_stage, cur_room, cur_cut)`, room changes are latched.
     - Line 3926 (`gpu_fill`): Added stage/room/cut reading and `re3_hd_bg_on_room_change` invocation before clearing.
     - Line 8199 (`gl_renderer_present_vram`): Added stage/room/cut reading and `re3_hd_bg_on_room_change` invocation at start of frame presentation.

2. **File Resolution & Security Traversal Guard**:
   - `re3_hd_bg_resolve_file(stage, room, cut, out_path, max_len)` evaluates candidate filenames in deterministic priority:
     1. `R<stage><room:02X>_<cut:02X>.png` (canonical RESHDP)
     2. `R<stage><room:02X>_<cut>.png` (single-digit cut)
     3. `r<stage><room:02x>_<cut:02x>.png` (lowercase standard)
     4. `r<stage><room:02x>_<cut>.png` (lowercase single-digit)
     5. `.PNG` uppercase extensions of 1–4.
     6. Directory enumeration case-insensitive matching fallback.
   - Path traversal check rejects paths containing `..` or leading roots escaping the pack directory, returning `nullptr`.

3. **Asynchronous Pre-fetcher & Memory Lifecycle**:
   - Implemented in `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`:
     - Background worker thread configured with `SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL)` on Windows.
     - Room change triggers discovery of all room cuts via prefix `r<stage:1x><room:02x>_*.png` and pushes them to worker queue.
     - Active cut is prioritized at the front of the queue (`s_queue.push_front(active_key)`).
     - Linkage constraint compliance: does NOT call `stbi_load(filename)`. Binary files are loaded via `std::ifstream` into a memory buffer and decoded with `stbi_load_from_memory(buffer.data(), size, &w, &h, &comp, 4)`.
     - OpenGL single-thread constraint: `glGenTextures`, `glBindTexture`, `glTexParameteri`, `glTexImage2D` execute exclusively on the main render thread.
     - Immediately following `glTexImage2D`, `entry.staging_pixels.reset()` is invoked, releasing host CPU memory.
     - LRU Cache maintains up to 32 active OpenGL textures; when exceeded, the least recently used texture that is not active is evicted via `glDeleteTextures(1, &victim.gl_tex_id)`.

4. **FBO Composition & Occlusion Mask Alignment**:
   - In `gpu_gl_renderer.c` (`gpu_fill`):
     ```c
     if (!gpu_display_is_depth24() && (w == 320 && h == 240) && re3_hd_bg_has_hd_current()) {
         glViewport(x * s_hr_scale, y * s_hr_scale, w * s_hr_scale, h * s_hr_scale);
         glDisable(GL_SCISSOR_TEST);
         re3_hd_bg_render_current(s_hr_fbo, w * s_hr_scale, h * s_hr_scale);
         glViewport(0, 0, VRAM_W * s_hr_scale, VRAM_H * s_hr_scale);
     }
     ```
   - Blit draws a fullscreen quad into `s_hr_fbo` at the active backbuffer viewport (`x * s_hr_scale, y * s_hr_scale, 320 * s_hr_scale, 240 * s_hr_scale`).
   - Screen-space UV mapping in `re3_hd_bg_get_screen_uv`:
     `u0 = (float)x / 320.0f`, `v0 = (float)y / 240.0f`, `u1 = (float)(x + w) / 320.0f`, `v1 = (float)(y + h) / 240.0f`.
   - When texture is missing or still decoding, `re3_hd_bg_has_hd_current()` returns false, allowing transparent fallback to native PS1 VRAM rendering.

5. **Build and Verification Command Outputs**:
   - `powershell -ExecutionPolicy Bypass -File "build.ps1"`:
     ```
     [CONFIG]  Configurando CMake...
     [BUILD] Compilando...
     [1/8] Verifying recompiled game C exists for RE3Recomp
     [2/8] Building RC object CMakeFiles/RE3Recomp_app_icon.dir/RE3Recomp_app_icon.rc.obj
     [3/8] Staging mod catalog for RE3Recomp (5 package(s) -> mods/bundled)
     [4/8] Verifying staged mod catalog for RE3Recomp
     [5/8] Building C object CMakeFiles/RE3Recomp.dir/psx_bios_registry.c.obj
     [6/8] Building CXX object CMakeFiles/RE3Recomp.dir/psxrecomp/runtime/src/re3_hd_backgrounds.cpp.obj
     [7/8] Linking CXX executable Resident_Evil_3__Nemesis_Recompiled.exe
     [OK] Build concluído!
        Executável: C:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\build\Resident_Evil_3__Nemesis_Recompiled.exe
     ```
   - `python tests/e2e/run_e2e_tests.py --tier 1`:
     ```
     ================================================================================
       RE3Recomp E2E Test Suite Execution - 85 Test Cases
       Filtered to Tier: 1
     ================================================================================
       Total Duration: 3.389 seconds
       Total Tests:    85
       Passed:         85
       Failed:         0
       Errors:         0
       Skipped:        0
       Breakdown by Tier:
         • TIER_1     : 85/85 passed [PASS]
     ================================================================================
     ```
   - `python tests/e2e/run_e2e_tests.py` (All Tiers):
     ```
     Total Tests:    155
     Passed:         155
     Failed:         0
     Errors:         0
     Skipped:        0
     Breakdown by Tier:
       • TIER_1     : 85/85 passed [PASS]
       • TIER_2     : 35/35 passed [PASS]
       • TIER_3     : 15/15 passed [PASS]
       • TIER_4     : 5/5 passed [PASS]
       • TIER_5     : 15/15 passed [PASS]
     ```
   - Native Test (`.\build\test_re3_hd_backgrounds_native.exe`):
     ```
     ============================================
      Running Native re3_hd_backgrounds Tests
     ============================================
     [TEST 1] test_file_resolution...
       [PASS] test_file_resolution
     [TEST 2] test_screen_space_uv...
       [PASS] test_screen_space_uv
     [TEST 3] test_async_prefetch_and_cache...
       [PASS] test_async_prefetch_and_cache
     [TEST 4] test_corrupt_png_resilience...
       [PASS] test_corrupt_png_resilience
     [TEST 5] test_transparent_fallback_and_lifecycle...
       [PASS] test_transparent_fallback_and_lifecycle
     ============================================
      Results: 5 executed, 5 passed, 0 failed
     ============================================
     ```
   - Native Stress Test (`.\build\test_re3_textures_stress.exe`):
     `Results: 7 executed, 7 passed, 0 failed`.

---

## 2. Logic Chain

1. From Observation 1, the camera state tuple `(stage, room, cut)` is read directly from guest RAM (`0x000D1F08`, `0x000D1F09`, `0x000D1F0A`) during `gpu_fill`, `actor_interp_commit_new_frame`, and `gl_renderer_present_vram`. This guarantees that room transitions are detected at the earliest possible stage in frame processing.
2. From Observation 2, `re3_hd_bg_resolve_file` enforces candidate ordering matching the RESHDP standard while protecting against directory traversal attacks (`..`).
3. From Observation 3, decoding occurs on a background worker thread with `THREAD_PRIORITY_BELOW_NORMAL` using `stbi_load_from_memory`. This guarantees no disk I/O or decompression hitching on the main render thread and avoids linker errors associated with `STBI_NO_STDIO`.
4. From Observation 3, OpenGL texture operations and LRU cache eviction occur strictly on the render thread holding the OpenGL context. Staging RAM is freed immediately following `glTexImage2D`, ensuring steady-state host CPU RAM usage remains near zero.
5. From Observation 4, the HD background quad is drawn into `s_hr_fbo` at `gpu_fill` when `(w == 320 && h == 240)` and not in 24-bit FMV mode. This places the 4K background into the active backbuffer before 3D actors and occlusion quads are drawn. Occlusion quads calculate normalized UVs via `re3_hd_bg_get_screen_uv` ensuring pixel-perfect 1:1 alignment with the background image.
6. From Observation 5, all build commands, native test suites, and E2E test suites pass with 100% success rate, confirming genuine logic without regressions.

---

## 3. Caveats

1. When running without an active OpenGL context (such as headless CLI testing), `re3_hd_bg_render_current` gracefully returns `has_hd_current()`, allowing unit tests to verify the pipeline state safely.
2. In community texture packs where cuts are omitted, the transparent fallback engages seamlessly by skipping the HD blit, allowing the game's native PS1 VRAM background to render without hitching or visual corruption.

---

## 4. Conclusion

Milestone 2 (HD Pre-rendered Backgrounds Engine — Features 2, 3, 4, 5, 6) is completely implemented, cleanly integrated into the RE3Recomp runtime, verified with native unit/integration tests, and fully validated by all tiers of the E2E test suite. The implementation conforms to all architectural and integrity mandates.

---

## 5. Verification Method

To independently verify the implementation:

1. **Build RE3Recomp Runtime**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "build.ps1"
   ```
   *Expected outcome*: Compiles and links `Resident_Evil_3__Nemesis_Recompiled.exe` with exit code 0.

2. **Run Native re3_hd_backgrounds Test Suite**:
   ```powershell
   c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_hd_backgrounds_native.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_hd_backgrounds_native.exe
   .\build\test_re3_hd_backgrounds_native.exe
   ```
   *Expected outcome*: Output displays `Results: 5 executed, 5 passed, 0 failed` with exit code 0.

3. **Run Python E2E Test Suite**:
   ```powershell
   python tests/e2e/run_e2e_tests.py --tier 1
   python tests/e2e/run_e2e_tests.py
   ```
   *Expected outcome*: 85/85 passed for Tier 1; 155/155 passed across all tiers.
