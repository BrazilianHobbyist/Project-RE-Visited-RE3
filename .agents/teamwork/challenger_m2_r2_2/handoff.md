# Adversarial Challenge Handoff Report: Challenger M2 R2 2

## Mission: Verify Resolution of Key=0 Sentinel Defect & Adversarial Resilience
**Verdict**: **CONFIRM CORRECTNESS** (All 6/6 adversarial tests passed; Key=0 sentinel defect verified resolved)

---

## 1. Observation

1. **Compilation and Execution of Native Adversarial Test (`tests/test_re3_m2_adversarial.cpp`)**:
   - Compilation command:
     ```powershell
     c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m2_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_m2_adversarial.exe
     ```
     Result: Exit code 0, 0 compiler warnings/errors.
   - Execution command:
     ```powershell
     .\build\test_re3_m2_adversarial.exe
     ```
   - Verbatim console output:
     ```
     ===============================================================
      RE3Recomp M2: Adversarial Fault Injection & Resilience Harness
     ===============================================================
     [ADVERSARIAL TEST 1] test_adv_malformed_png_injection...
     psxrecomp: [Textures] Active pack: "MalformPack" at "C:/Users/victo/AppData/Local/Temp/re3_malform_41_18467/mods/textures/MalformPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
     psxrecomp: [Textures] Subsystem shutdown complete.
       [PASS] test_adv_malformed_png_injection
     [ADVERSARIAL TEST 2] test_adv_path_traversal_injection...
       [PASS] test_adv_path_traversal_injection
     [ADVERSARIAL TEST 3] test_adv_extreme_out_of_bounds...
     psxrecomp: [Textures] Active pack: "OobPack" at "C:/Users/victo/AppData/Local/Temp/re3_oob_19169_15724/mods/textures/OobPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
         DEBUG: res0 = C:/Users/victo/AppData/Local/Temp/re3_oob_19169_15724/mods/textures/OobPack/backgrounds/R000_00.png
         DEBUG: is_pack_valid = 1, hd_backgrounds = 1
         Observation: stage 0, room 0, cut 1 (key = 1): wait_for_decode = 1, has_hd = 1 [SUCCESS]
         Observation: stage 0, room 0, cut 0 (key = 0): wait_for_decode = 1, has_hd = 1 [FAILURE]
     psxrecomp: [Textures] Subsystem shutdown complete.
       [PASS] test_adv_extreme_out_of_bounds
     [ADVERSARIAL TEST 4] test_adv_reshdp_variations_and_precedence...
       [PASS] test_adv_reshdp_variations_and_precedence
     [ADVERSARIAL TEST 5] test_adv_partial_packs_transparent_fallback...
     psxrecomp: [Textures] Active pack: "PartialPack" at "C:/Users/victo/AppData/Local/Temp/re3_partial_26962_24464/mods/textures/PartialPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
     psxrecomp: [Textures] Subsystem shutdown complete.
       [PASS] test_adv_partial_packs_transparent_fallback
     [ADVERSARIAL TEST 6] test_adv_cache_thrashing_and_concurrency...
     psxrecomp: [Textures] Active pack: "StressPack" at "C:/Users/victo/AppData/Local/Temp/re3_stress_5705_28145/mods/textures/StressPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
     psxrecomp: [Textures] Subsystem shutdown complete.
       [PASS] test_adv_cache_thrashing_and_concurrency
     ===============================================================
      Adversarial Results: 6 executed, 6 passed, 0 failed
     ===============================================================
     ```
     Result: Exit code 0.

2. **Empirical Verification of Key=0 Sentinel Resolution in Test 3 (`test_adv_extreme_out_of_bounds`)**:
   - For `stage = 0, room = 0, cut = 0` (key = 0):
     - `wait_for_decode = 1` (completed within 500ms timeout)
     - `has_hd = 1` (verified present in cache with valid state)
     - The previous bug check `if (!ok_d) { std::printf(" [BUG CONFIRMED] ..."); }` did NOT fire.
     - `ADV_TEST_ASSERT(ok_d == true, ...)` evaluated to `true`.
     - `[PASS] test_adv_extreme_out_of_bounds` achieved 100% pass rate.

3. **Source Code Inspection of `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`**:
   - Lines 38–39:
     ```cpp
     constexpr size_t kMaxCachedTextures = 32;
     constexpr uint32_t kInvalidBgKey = 0xFFFFFFFFu;
     ```
   - Lines 275–309 in `worker_thread_func()`:
     ```cpp
     while (!s_stop) {
         bool has_job = false;
         uint32_t key = kInvalidBgKey;
         std::string resolved_path;

         {
             std::unique_lock<std::mutex> lock(s_mutex);
             s_cv.wait(lock, [&] {
                 return s_stop.load() || !s_queue.empty();
             });

             if (s_stop) break;

             if (!s_queue.empty()) {
                 key = s_queue.front();
                 s_queue.pop_front();

                 auto it = s_entries.find(key);
                 if (it != s_entries.end()) {
                     if (it->second.state == BgState::ReadyGL || it->second.state == BgState::ReadyRAM) {
                         continue; // Already decoded/uploaded
                     }
                     if (it->second.filepath.empty()) {
                         it->second.state = BgState::Failed;
                         continue;
                     }
                     it->second.state = BgState::Loading;
                     resolved_path = it->second.filepath;
                     has_job = true;
                 }
             }
         }

         if (!has_job || resolved_path.empty()) continue;
     ```
   - The erroneous condition `if (key == 0 || resolved_path.empty()) continue;` has been replaced with `if (!has_job || resolved_path.empty()) continue;`.
   - `kInvalidBgKey` is initialized to `0xFFFFFFFFu`, which never collides with any valid `uint32_t` key (valid keys are bounded by bits 0..23, max `0x00FFFFFF`).

4. **Remediation of LRU Eviction Zombie State & Evicted Cut Re-acquisition**:
   - In `upload_entry_to_gl()` (lines 365–375):
     ```cpp
     auto ent_it = s_entries.find(victim_key);
     if (ent_it != s_entries.end() && ent_it->second.gl_tex_id != 0) {
         glDeleteTextures(1, &ent_it->second.gl_tex_id);
         ent_it->second.gl_tex_id = 0;
         ent_it->second.staging_pixels.reset();
         s_metrics_evictions++;
         s_lru_list.erase(std::next(it).base());
         s_entries.erase(ent_it);
         break;
     }
     ```
   - Victim entries are properly purged from `s_entries` upon eviction.
   - When a player returns to an evicted cut, `re3_hd_bg_on_room_change()` (lines 645–660) discovers `act_it == s_entries.end()`, creates a new entry, and prepends it to `s_queue.push_front(active_key)`.
   - Concurrency stress test execution command:
     ```powershell
     clang++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_hd_backgrounds_stress.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -lpsapi -o build/test_re3_hd_backgrounds_stress.exe
     .\build\test_re3_hd_backgrounds_stress.exe
     ```
   - Result:
     ```
     [TEST 4] test_stress_lru_eviction_boundary_and_reacquisition...
         Cut 0 successfully uploaded with GL tex ID 1
         After 33 texture uploads: evictions=1
         Switching back to evicted Cut 0...
         Re-acquisition check: has_hd_current() = 1
         Re-acquisition check: get_current_gl_tex() = 1
       [PASS] test_stress_lru_eviction_boundary_and_reacquisition
     ============================================================
      Stress Results: 5 executed, 5 passed, 0 failed
     ============================================================
     ```

5. **Additional Regression and Integration Verification**:
   - `build/test_re3_hd_backgrounds_native.exe`: 5 executed, 5 passed, 0 failed.
   - Python E2E Suite (`python tests/e2e/run_e2e_tests.py`): 155 executed, 155 passed, 0 failed.
   - Full Runtime Build (`powershell -ExecutionPolicy Bypass -File "build.ps1"`): Exit code 0, binary created at `build/Resident_Evil_3__Nemesis_Recompiled.exe`.

---

## 2. Logic Chain

1. From Observation 3, the worker thread in `psxrecomp/runtime/src/re3_hd_backgrounds.cpp` was refactored to use an explicit `bool has_job` state variable and sentinel constant `kInvalidBgKey = 0xFFFFFFFFu`.
2. From Observation 3, valid keys use at most 24 bits (`stage << 16 | room << 8 | cut`), making `0xFFFFFFFF` an unambiguous non-colliding sentinel.
3. From Observation 3, when `stage=0, room=0, cut=0` is processed, `key` is `0`, `has_job` evaluates to `true`, and the worker thread enters `decode_png_file()`.
4. From Observation 1 and Observation 2, `test_adv_extreme_out_of_bounds` demonstrates that `wait_for_decode` returns `1` and `has_hd` returns `1` for `(0, 0, 0)`, proving the decoded image transitions into `BgState::ReadyRAM` and is available for GL texture upload.
5. From Observation 1, all 6 adversarial test suites (malformed PNG injection, path traversal, out-of-bounds, RESHDP variations, partial pack fallback, cache thrashing and concurrency) completed with 0 failures.
6. From Observation 4 and Observation 5, eviction memory bounding, LRU recency promotion, native unit tests, Python E2E integration tests, and full project build succeed without regressions.
7. Therefore, the defect reported in Round 1 has been completely resolved and Milestone M2 meets all adversarial resilience requirements.

---

## 3. Caveats

1. The test print string in line 268 of `tests/test_re3_m2_adversarial.cpp` contains static text `"Observation: stage 0, room 0, cut 0 (key = 0): wait_for_decode = %d, has_hd = %d [FAILURE]"` that was placed there during the original defect investigation. The boolean values printed are `wait_for_decode = 1, has_hd = 1`, and the actual assert `ADV_TEST_ASSERT(ok_d == true, ...)` succeeds, confirming the test logic passes.
2. Hardware OpenGL blitting was tested in a headless context; OpenGL draw calls bypass rendering safely when no active context is present.

---

## 4. Conclusion

**Verdict**: **CONFIRM CORRECTNESS**

- **Key=0 Sentinel Defect**: RESOLVED. Backgrounds for stage 0, room 0, cut 0 (`R000_00.png`) decode, cache, and report active state correctly.
- **LRU Cache Eviction**: RESOLVED. Entries are evicted and re-acquired cleanly without starvation or leaks.
- **Adversarial Resilience**: CONFIRMED. 6/6 adversarial tests passed (100% success rate).
- **Stress & Concurrency**: CONFIRMED. 5/5 stress tests passed (100% success rate).
- **Regression Suite**: CONFIRMED. 155/155 Python E2E tests passed; full project builds cleanly.

Milestone M2 is fully verified and cleared for progression.

---

## 5. Verification Method

To independently verify this result:

1. **Compile and Run Native Adversarial Test Suite**:
   ```powershell
   c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m2_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_m2_adversarial.exe
   .\build\test_re3_m2_adversarial.exe
   ```
   *Expected outcome*: `Adversarial Results: 6 executed, 6 passed, 0 failed` (Exit code 0).

2. **Compile and Run Concurrency Stress Harness**:
   ```powershell
   clang++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_hd_backgrounds_stress.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -lpsapi -o build/test_re3_hd_backgrounds_stress.exe
   .\build\test_re3_hd_backgrounds_stress.exe
   ```
   *Expected outcome*: `Stress Results: 5 executed, 5 passed, 0 failed` (Exit code 0).

3. **Run Full Project Build**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "build.ps1"
   ```
   *Expected outcome*: Clean build with exit code 0.
