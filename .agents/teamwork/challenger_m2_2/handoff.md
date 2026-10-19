# Adversarial Challenge Handoff Report: Challenger M2 2

## Mission: Fault Injection & Edge-Case Resilience (Milestone M2)
**Verdict**: **REPORT FAILURE** (Defect empirically reproduced in `psxrecomp/runtime/src/re3_hd_backgrounds.cpp:301`)

---

## 1. Observation

1. **Bug Reproduction in `re3_hd_backgrounds.cpp`**:
   - In `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`, the background cache key is computed as:
     ```cpp
     inline uint32_t bg_make_key(uint8_t stage, uint8_t room, uint8_t cut) {
         return (static_cast<uint32_t>(stage) << 16) |
                (static_cast<uint32_t>(room) << 8) |
                static_cast<uint32_t>(cut);
     }
     ```
   - For `stage = 0, room = 0, cut = 0` (Resident Evil 3's initial debug/prologue boundary room angle `R000_00.png`), `bg_make_key(0, 0, 0)` returns `0`.
   - In `worker_thread_func()` (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp:275-301`):
     ```cpp
     while (!s_stop) {
         uint32_t key = 0;
         std::string resolved_path;
         {
             std::unique_lock<std::mutex> lock(s_mutex);
             s_cv.wait(lock, [&] { return s_stop.load() || !s_queue.empty(); });
             if (s_stop) break;
             if (!s_queue.empty()) {
                 key = s_queue.front();
                 s_queue.pop_front();
                 auto it = s_entries.find(key);
                 if (it != s_entries.end()) {
                     if (it->second.state == BgState::ReadyGL || it->second.state == BgState::ReadyRAM) {
                         continue;
                     }
                     it->second.state = BgState::Loading;
                     resolved_path = it->second.filepath;
                 }
             }
         }

         if (key == 0 || resolved_path.empty()) continue; // <-- LINE 301 DEFECT
     ```
   - When key is `0`, line 301 evaluates `(key == 0)` to `true` and executes `continue;`.
   - This discards the popped job without invoking `decode_png_file()`.
   - The entry in `s_entries` remains stranded in `BgState::Loading` indefinitely.
   - Subsequent calls to `re3_hd_bg_has_hd_current()` check `(it->second.gl_tex_id != 0 || it->second.state == BgState::ReadyRAM)` and return `false`.
   - Consequently, any texture pack providing `R000_00.png` for stage 0, room 0, cut 0 is permanently ignored by the engine and silently dropped.

2. **Empirical Execution of Native Adversarial Test (`tests/test_re3_m2_adversarial.cpp`)**:
   - Test command:
     ```powershell
     c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m2_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_m2_adversarial.exe
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
         Observation: stage 0, room 0, cut 0 (key = 0): wait_for_decode = 0, has_hd = 0 [FAILURE]
         [BUG CONFIRMED] re3_hd_backgrounds.cpp:301 skips decoding when key == 0 (stage 0, room 0, cut 0).
       [FAIL] Stage 0, room 0, cut 0 must load (Failed: key == 0 discarded by worker thread) (Line 273: ok_d == true)
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
      Adversarial Results: 6 executed, 5 passed, 1 failed
     ===============================================================
     ```

3. **Resilience Verification on Other Adversarial Vectors**:
   - **Malformed/corrupted PNGs** (zero-byte, 1-byte, 7-byte, header-only, corrupted IHDR, fake formats like GIF/BMP/JPEG/text, damaged IDAT, 65MB file): Zero crashes occurred; all invalid inputs cleanly evaluated to `has_hd_current() == false` and triggered transparent fallback. Subsequent valid cuts continued to decode normally (fault isolation verified).
   - **Path traversal injection** (`../../windows/system32`, `..\\..\\secret`, `C:\Windows\System32`, UNC paths): Cleanly blocked by `re3_hd_bg_resolve_file` (`norm_bg` vs `norm_pack` canonical boundary check and `..` check).
   - **RESHDP filename variations**: Successfully matched canonical double-digit (`R100_00.png`), single-digit (`R100_1.png`), lowercase (`r100_02.png`), lowercase single-digit (`r100_3.png`), uppercase extension (`.PNG`), and hexadecimal rooms/cuts (`R10A_0F.png`). When both canonical and single-digit exist on disk, canonical takes precedence.
   - **Partial packs**: Transparent fallback verified. Missing cuts returned `has_hd_current() == false` without latency, while available cuts hit cache immediately.
   - **Cache thrashing & concurrency**: Hammering background state across concurrent threads while cycling 45 cuts (> 32 limit) executed without deadlocks or memory corruption.

4. **Python Simulator E2E Suite Output (`tests/e2e/test_tier5_adversarial.py`)**:
   - Command: `python -m unittest tests/e2e/test_tier5_adversarial.py`
   - Result: 15/15 passed in 4.466s.
   - Note: The Python simulator uses string keys (e.g. `"R000_00"`), which masked the integer `key == 0` sentinel defect present in the native C++ engine.

---

## 2. Logic Chain

1. From Observation 1, the C++ runtime uses `(stage << 16) | (room << 8) | cut` to represent background keys as integer `uint32_t`.
2. From Observation 1, `(stage=0, room=0, cut=0)` computes to `0`.
3. From Observation 1, `psxrecomp/runtime/src/re3_hd_backgrounds.cpp:301` checks `if (key == 0 || resolved_path.empty()) continue;`.
4. From Observation 1, line 301 treats `key == 0` as a sentinel indicating that no valid work item was dequeued, even though key 0 was legitimately dequeued from `s_queue`.
5. From Observation 2, test `test_adv_extreme_out_of_bounds` proves that `stage 0, room 0, cut 1` (key 1) succeeds in decoding and reports `has_hd = 1`, whereas `stage 0, room 0, cut 0` (key 0) consistently fails with `has_hd = 0` and `wait_for_decode = 0`.
6. From Observation 3, all other stress dimensions (malformed PNGs, path traversal, filename variations, partial packs, cache eviction) demonstrated robust resilience.
7. Therefore, Milestone M2 contains a reproducible defect on lower boundary room angles `(0, 0, 0)` that prevents HD background display.

---

## 3. Caveats

1. The defect does not cause a crash, memory leak, or hang; it fails safely into PS1 VRAM fallback, but fails to deliver the HD background for `(stage=0, room=0, cut=0)`.
2. Hardware OpenGL blitting was tested in headless context; actual rendering pipeline in headless CLI mode skips GL draw calls cleanly as designed.
3. The Python test suite `tests/e2e/test_tier5_adversarial.py` tests simulator models and did not catch this native C++ type assumption.

---

## 4. Conclusion & Recommended Mitigation

**Verdict**: **REPORT FAILURE**

### Defect Summary:
- **Location**: `psxrecomp/runtime/src/re3_hd_backgrounds.cpp:301`
- **Issue**: Sentinel check `if (key == 0 || resolved_path.empty()) continue;` incorrectly drops background decoding for `(stage 0, room 0, cut 0)`.
- **Severity**: MEDIUM-HIGH (Silent failure to load HD backgrounds for any room with stage 0, room 0, cut 0).

### Recommended Mitigation:
In `psxrecomp/runtime/src/re3_hd_backgrounds.cpp:worker_thread_func`:
Replace the sentinel pattern with an explicit boolean flag:
```cpp
bool has_job = false;
uint32_t key = 0;
std::string resolved_path;

{
    std::unique_lock<std::mutex> lock(s_mutex);
    s_cv.wait(lock, [&] { return s_stop.load() || !s_queue.empty(); });
    if (s_stop) break;

    if (!s_queue.empty()) {
        key = s_queue.front();
        s_queue.pop_front();
        has_job = true;

        auto it = s_entries.find(key);
        if (it != s_entries.end()) {
            if (it->second.state == BgState::ReadyGL || it->second.state == BgState::ReadyRAM) {
                continue;
            }
            it->second.state = BgState::Loading;
            resolved_path = it->second.filepath;
        }
    }
}

if (!has_job || resolved_path.empty()) continue;
```
Alternatively, initialize `key = 0xFFFFFFFF` as sentinel, since valid keys only occupy bits 0..23 (`(stage << 16) | (room << 8) | cut`).

---

## 5. Verification Method

To independently reproduce and verify this finding:

1. **Compile Native Adversarial Test Suite**:
   ```powershell
   c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m2_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_m2_adversarial.exe
   ```

2. **Execute Native Adversarial Test Binary**:
   ```powershell
   .\build\test_re3_m2_adversarial.exe
   ```
   *Expected outcome*:
   - `[ADVERSARIAL TEST 1] test_adv_malformed_png_injection... [PASS]`
   - `[ADVERSARIAL TEST 2] test_adv_path_traversal_injection... [PASS]`
   - `[ADVERSARIAL TEST 3] test_adv_extreme_out_of_bounds... [FAIL]`
     - Line output: `Observation: stage 0, room 0, cut 1 (key = 1): wait_for_decode = 1, has_hd = 1 [SUCCESS]`
     - Line output: `Observation: stage 0, room 0, cut 0 (key = 0): wait_for_decode = 0, has_hd = 0 [FAILURE]`
     - Line output: `[BUG CONFIRMED] re3_hd_backgrounds.cpp:301 skips decoding when key == 0 (stage 0, room 0, cut 0).`
   - Exit code: 1.

3. **Invalidation Condition**:
   - Applying the proposed mitigation allows `[ADVERSARIAL TEST 3]` to pass with 6/6 passed and exit code 0.
