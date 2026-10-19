# Handoff Report: Challenger M2 1 — Concurrency & Stress Testing

## 1. Observation

1. **Test Harness Compilation & Execution Command**:
   Compiled and executed `tests/test_re3_hd_backgrounds_stress.cpp` against the RE3Recomp runtime, linked with SDL3, OpenGL, and psapi:
   ```powershell
   clang++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_hd_backgrounds_stress.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -lpsapi -o build/test_re3_hd_backgrounds_stress.exe
   .\build\test_re3_hd_backgrounds_stress.exe
   ```

2. **Verbatim Execution Output**:
   ```
   ============================================================
    RE3Recomp M2: HD Backgrounds Concurrency & Stress Harness  
   ============================================================
   [TEST 1] test_stress_rapid_init_shutdown_cycles...
   psxrecomp: [Textures] Active pack: "CyclePack" at "C:/Users/victo/AppData/Local/Temp/re3_m2_stress_41_18467/mods/textures/CyclePack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
       Successfully executed 200 rapid init/shutdown cycles with zero deadlocks!
   psxrecomp: [Textures] Subsystem shutdown complete.
     [PASS] test_stress_rapid_init_shutdown_cycles
   [TEST 2] test_stress_concurrent_room_changes...
   psxrecomp: [Textures] Active pack: "StressPack" at "C:/Users/victo/AppData/Local/Temp/re3_m2_stress_6334_26500/mods/textures/StressPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
       Completed 1000 rapid concurrent room changes across 10 threads!
       Metrics: hits=0, misses=1000, prefetches=120, evictions=0, entries=120
   psxrecomp: [Textures] Subsystem shutdown complete.
     [PASS] test_stress_concurrent_room_changes
   [TEST 3] test_stress_worker_decode_vs_query_race...
   psxrecomp: [Textures] Active pack: "RacePack" at "C:/Users/victo/AppData/Local/Temp/re3_m2_stress_19169_15724/mods/textures/RacePack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
   psxrecomp: [Textures] Subsystem shutdown complete.
     [PASS] test_stress_worker_decode_vs_query_race
   [TEST 4] test_stress_lru_eviction_boundary_and_reacquisition...
   psxrecomp: [Textures] Active pack: "LruPack" at "C:/Users/victo/AppData/Local/Temp/re3_m2_stress_11478_29358/mods/textures/LruPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
       Cut 0 successfully uploaded with GL tex ID 1
       After 33 texture uploads: evictions=1
       Switching back to evicted Cut 0...
       Re-acquisition check: has_hd_current() = 0
       Re-acquisition check: get_current_gl_tex() = 0
     [FAIL] CRITICAL: Evicted Cut 0 MUST be reloadable upon re-visiting! (Line 322: cut0_reacquired == true)
   [TEST 5] test_stress_memory_boundedness...
   psxrecomp: [Textures] Active pack: "MemPack" at "C:/Users/victo/AppData/Local/Temp/re3_m2_stress_26962_24464/mods/textures/MemPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
       Memory Start: 62244 KB, End: 62256 KB (Diff: +12 KB)
   psxrecomp: [Textures] Subsystem shutdown complete.
     [PASS] test_stress_memory_boundedness
   ============================================================
    Stress Results: 5 executed, 4 passed, 1 failed
   ============================================================
   ```

3. **Code Defect in `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`**:
   - Lines 360-366 in `upload_entry_to_gl`:
     ```cpp
     glDeleteTextures(1, &ent_it->second.gl_tex_id);
     ent_it->second.gl_tex_id = 0;
     ent_it->second.state = BgState::None;
     s_metrics_evictions++;
     s_lru_list.erase(std::next(it).base());
     break;
     ```
     `ent_it` has its OpenGL texture deleted and `state` set to `BgState::None`, but the entry is **never erased from `s_entries`**.
   - Lines 602 and 624 in `re3_hd_bg_on_room_change`:
     ```cpp
     // Line 602
     if (s_entries.find(k) == s_entries.end()) { ... }
     ...
     // Line 624
     if (s_entries.find(active_key) == s_entries.end()) { ... }
     ```
     Because the evicted entry key still exists in `s_entries`, both checks evaluate to `false`. The evicted cut is **never re-resolved and never queued again**.
   - Lines 650-654 in `re3_hd_bg_has_hd_current()`:
     ```cpp
     auto it = s_entries.find(key);
     if (it == s_entries.end()) return false;
     return (it->second.gl_tex_id != 0 || it->second.state == BgState::ReadyRAM);
     ```
     Since `gl_tex_id == 0` and `state == BgState::None`, `has_hd_current()` permanently returns `false`.
   - Lines 664-666 in `re3_hd_bg_get_current_gl_tex()`:
     ```cpp
     if (it->second.gl_tex_id == 0 && it->second.state == BgState::ReadyRAM) {
         upload_entry_to_gl(it->second);
     }
     s_active_gl_tex = it->second.gl_tex_id;
     return s_active_gl_tex;
     ```
     Since `state != BgState::ReadyRAM`, `upload_entry_to_gl` is never called, and `get_current_gl_tex()` permanently returns 0.

---

## 2. Logic Chain

1. From Observation 1 and 2, when 33 textures are loaded, the cache capacity limit of 32 (`kMaxCachedTextures`) is reached and eviction is triggered (`evictions = 1`).
2. From Observation 3 (lines 360–366), eviction sets `ent_it->second.gl_tex_id = 0` and `ent_it->second.state = BgState::None`, but leaves `ent_it` inside `s_entries`.
3. From Observation 3 (lines 602 & 624), upon re-visiting the evicted room angle (Cut 0), `re3_hd_bg_on_room_change` checks `s_entries.find(active_key) == s_entries.end()`. Since `active_key` is present in `s_entries`, it completely skips resolving the file on disk and skips adding `active_key` to `s_queue`.
4. Because `s_queue` is never updated, the background worker thread is never signaled and never decodes Cut 0.
5. In Observation 2, polling `re3_hd_bg_has_hd_current()` for Cut 0 returns `false` (0) and `re3_hd_bg_get_current_gl_tex()` returns 0 indefinitely.
6. Therefore, any texture evicted from the LRU cache is permanently lost for the entire lifetime of the game session. The game permanently falls back to original PS1 backgrounds for any previously visited room/cut that was evicted.

---

## 3. Caveats

1. Multi-threading and concurrency primitives (`std::mutex`, `std::condition_variable`, `std::atomic<bool>`) demonstrated zero deadlocks and zero crashes across 1000 concurrent room changes over 10 threads, 200 rapid init/shutdown cycles, and simultaneous worker decode vs main thread queries.
2. The flaw is purely functional and architectural within the LRU eviction and re-queueing state machine.

---

## 4. Conclusion

**VERDICT: REPORT FAILURE**

The implementation of Milestone 2 contains a critical bug:
- **LRU Cache Eviction Permanent Starvation**: Once the 32-entry cache threshold is exceeded, evicted camera cuts cannot be re-acquired when the player returns to them. The background permanently reverts to low-resolution PS1 rendering.
- **Secondary LRU Recency Defect**: On cache hits (`gl_tex_id != 0`), `s_lru_list` is never updated. Only freshly uploaded textures update the LRU list, causing frequently viewed textures to be prematurely evicted.

### Required Worker Fixes:
1. In `upload_entry_to_gl` (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp`):
   When an entry is evicted, cleanly remove it from `s_entries`:
   ```cpp
   glDeleteTextures(1, &ent_it->second.gl_tex_id);
   s_metrics_evictions++;
   s_lru_list.erase(std::next(it).base());
   s_entries.erase(ent_it); // Allow subsequent on_room_change calls to re-queue
   break;
   ```
2. In `re3_hd_bg_on_room_change`:
   Guard both prefetch and active cut queueing with `state == BgState::None` support:
   ```cpp
   auto it = s_entries.find(k);
   if (it == s_entries.end() || it->second.state == BgState::None) { ... }
   ```
3. In `re3_hd_bg_get_current_gl_tex()`:
   Update LRU list on cache hits to maintain proper recency:
   ```cpp
   if (it->second.gl_tex_id != 0) {
       s_lru_list.remove(key);
       s_lru_list.push_front(key);
   }
   ```

---

## 5. Verification Method

1. **Re-run the Adversarial Stress Test**:
   ```powershell
   clang++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_hd_backgrounds_stress.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -lpsapi -o build/test_re3_hd_backgrounds_stress.exe
   .\build\test_re3_hd_backgrounds_stress.exe
   ```
2. **Invalidation Condition**:
   The failure verdict is invalidated if and only if `[TEST 4] test_stress_lru_eviction_boundary_and_reacquisition` passes with `5 executed, 5 passed, 0 failed` and exit code 0.
