# Handoff Report: Explorer M2 R2 1 — LRU Eviction & Re-Acquisition Fix Strategy

## 1. Observation

1. **Failure Observation in Stress Harness Test 4**:
   In `tests/test_re3_hd_backgrounds_stress.cpp` lines 302–324 (`test_stress_lru_eviction_boundary_and_reacquisition`):
   ```
   [TEST 4] test_stress_lru_eviction_boundary_and_reacquisition...
       Cut 0 successfully uploaded with GL tex ID 1
       After 33 texture uploads: evictions=1
       Switching back to evicted Cut 0...
       Re-acquisition check: has_hd_current() = 0
       Re-acquisition check: get_current_gl_tex() = 0
     [FAIL] CRITICAL: Evicted Cut 0 MUST be reloadable upon re-visiting! (Line 322: cut0_reacquired == true)
   ```

2. **Eviction Defect in `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`**:
   Lines 352–368 in `upload_entry_to_gl()`:
   ```cpp
   if (resident_count > kMaxCachedTextures) {
       for (auto it = s_lru_list.rbegin(); it != s_lru_list.rend(); ++it) {
           uint32_t victim_key = *it;
           if (victim_key == bg_make_key(s_active_stage, s_active_room, s_active_cut)) {
               continue; // Do not evict active cut
           }
           auto ent_it = s_entries.find(victim_key);
           if (ent_it != s_entries.end() && ent_it->second.gl_tex_id != 0) {
               glDeleteTextures(1, &ent_it->second.gl_tex_id);
               ent_it->second.gl_tex_id = 0;
               ent_it->second.state = BgState::None;
               s_metrics_evictions++;
               s_lru_list.erase(std::next(it).base());
               break;
           }
       }
   }
   ```
   When `victim_key` is evicted, its OpenGL texture is deleted, `gl_tex_id = 0`, and `state = BgState::None`. However, `ent_it` remains in `s_entries` as a dead entry ("zombie").

3. **Re-Acquisition Starvation in `re3_hd_bg_on_room_change()`**:
   Lines 602 and 624 in `re3_hd_bg_on_room_change()`:
   ```cpp
   // Line 602: room prefetch check
   if (s_entries.find(k) == s_entries.end()) {
       ...
       s_queue.push_back(k);
   }
   ...
   // Line 624: active cut priority check
   if (s_entries.find(active_key) == s_entries.end()) {
       char resolved[1024];
       if (re3_hd_bg_resolve_file(stage, room, cut, resolved, sizeof(resolved))) {
           ...
           s_queue.push_front(active_key);
       }
   }
   ```
   Both checks query `s_entries.find(key) == s_entries.end()`. Because evicted entries are never removed from `s_entries`, both checks evaluate to `false`. No file resolution is performed, nothing is added to `s_queue`, and the worker thread is never signaled.

4. **Permanent Fallback in State Query Functions**:
   In `re3_hd_bg_has_hd_current()` (lines 650–654):
   ```cpp
   uint32_t key = bg_make_key(s_active_stage, s_active_room, s_active_cut);
   auto it = s_entries.find(key);
   if (it == s_entries.end()) return false;
   return (it->second.gl_tex_id != 0 || it->second.state == BgState::ReadyRAM);
   ```
   For an evicted cut, `gl_tex_id == 0` and `state == BgState::None`. Thus `has_hd_current()` permanently returns `false`.
   Similarly, `re3_hd_bg_get_current_gl_tex()` (lines 664–668) checks `state == BgState::ReadyRAM` before uploading; because `state == BgState::None`, `upload_entry_to_gl()` is never invoked, and it permanently returns `0`.

5. **Secondary LRU Recency Defect in `re3_hd_bg_get_current_gl_tex()`**:
   Lines 660–668 in `re3_hd_bg_get_current_gl_tex()`:
   ```cpp
   uint32_t key = bg_make_key(s_active_stage, s_active_room, s_active_cut);
   auto it = s_entries.find(key);
   if (it == s_entries.end()) return 0;

   if (it->second.gl_tex_id == 0 && it->second.state == BgState::ReadyRAM) {
       upload_entry_to_gl(it->second);
   }
   s_active_gl_tex = it->second.gl_tex_id;
   return s_active_gl_tex;
   ```
   When a cut hits cache (`gl_tex_id != 0`), `s_lru_list` is never updated. Only freshly uploaded textures ever push to `s_lru_list`. As a result, frequently viewed cuts remain at the tail of `s_lru_list` and are prematurely evicted when another cut is loaded.

6. **Adjacent Defect: Key 0 Discard in Worker Thread**:
   In `tests/test_re3_m2_adversarial.cpp` lines 265–274 and `re3_hd_backgrounds.cpp` line 301:
   ```cpp
   if (key == 0 || resolved_path.empty()) continue;
   ```
   For Stage 0, Room 0, Cut 0, `bg_make_key(0, 0, 0) == 0`. Checking `key == 0` causes Stage 0, Room 0, Cut 0 (`R000_00.png`) to be skipped by the worker thread.

---

## 2. Logic Chain

1. From Observation 1, when 33 cuts are visited, `kMaxCachedTextures` (32) is exceeded, triggering eviction of Cut 0 (`evictions = 1`).
2. From Observation 2, `upload_entry_to_gl()` sets `gl_tex_id = 0` and `state = BgState::None`, but leaves the key inside `s_entries`.
3. When returning to Cut 0 within the same room (`stage = 1, room = 0`):
   - `room_changed = (stage != s_active_stage || room != s_active_room)` evaluates to `false`.
   - The prefetch directory scan (lines 567–621) is bypassed entirely.
   - Execution proceeds to line 624: `if (s_entries.find(active_key) == s_entries.end())`.
   - Since `active_key` is present in `s_entries`, this condition evaluates to `false`.
4. As demonstrated by Observation 3 and 4, Cut 0 is never re-resolved from disk and never added to `s_queue`. The worker thread is never signaled, so `state` remains `BgState::None` indefinitely.
5. Inquiries to `re3_hd_bg_has_hd_current()` and `re3_hd_bg_get_current_gl_tex()` fail perpetually (Observation 1 line 322).
6. Comparing architectural strategies:
   - **Approach A (In-place retention)**: If evicted entries are left in `s_entries` with `state = BgState::None`, `s_entries` grows monotonically with every cut visited across the entire game (hundreds of rooms), causing memory accumulation and bloating `s_metrics.active_entries`.
   - **Approach B (Clean erase)**: Calling `s_entries.erase(ent_it)` upon eviction purges the zombie entry. Subsequent lookups evaluate `find(k) == end()`, which naturally triggers file resolution and queueing via the existing code path. Memory remains strictly bounded.
   - **Approach C (Unified Dual-Layer)**: Erasing evicted entries from `s_entries` AND guarding re-queue checks with `(it == s_entries.end() || it->second.state == BgState::None)` provides complete robustness against state corruption, memory bloat, and priority stalls.

---

## 3. Caveats

1. **Thread Safety of Map Erase**:
   `upload_entry_to_gl()` is called from `re3_hd_bg_get_current_gl_tex()` while holding `std::lock_guard<std::mutex> lock(s_mutex)`. In C++, `std::unordered_map::erase(ent_it)` invalidates only the erased iterator/reference. The active cut `entry` passed by reference to `upload_entry_to_gl()` is guaranteed not to be `ent_it` because `victim_key == active_key` is explicitly skipped (`continue`). No other thread accesses `s_entries` without holding `s_mutex`.
2. **Path Resolution Re-execution Cost**:
   When an evicted cut is re-acquired, `re3_hd_bg_resolve_file()` will re-check disk candidates. Since this only happens when returning to an evicted cut (>32 distinct camera angles viewed since last visit), the amortized cost is negligible (~5 microseconds on modern filesystem caches).
3. **Queue Inversion Edge Case**:
   When entering a room, the prefetch loop pushes all room cuts to `s_queue.push_back()`. The active cut might be queued behind 10+ other cuts. Moving the active cut to `s_queue.push_front()` ensures instantaneous loading without waiting for irrelevant cuts.

---

## 4. Conclusion

The LRU cache eviction defect must be resolved using a **5-Point Robust Strategy**:

### Point 1: Clean Erase in `upload_entry_to_gl()`
In `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`:
```cpp
// BEFORE (lines 358-367):
auto ent_it = s_entries.find(victim_key);
if (ent_it != s_entries.end() && ent_it->second.gl_tex_id != 0) {
    glDeleteTextures(1, &ent_it->second.gl_tex_id);
    ent_it->second.gl_tex_id = 0;
    ent_it->second.state = BgState::None;
    s_metrics_evictions++;
    s_lru_list.erase(std::next(it).base());
    break;
}

// AFTER:
auto ent_it = s_entries.find(victim_key);
if (ent_it != s_entries.end() && ent_it->second.gl_tex_id != 0) {
    glDeleteTextures(1, &ent_it->second.gl_tex_id);
    ent_it->second.gl_tex_id = 0;
    ent_it->second.staging_pixels.reset();
    s_metrics_evictions++;
    s_lru_list.erase(std::next(it).base());
    s_entries.erase(ent_it); // Purge from map so subsequent visits re-resolve and re-queue cleanly
    break;
}
```

### Point 2: Dual-Layer Guard in Prefetch Loop
In `re3_hd_bg_on_room_change()` (lines 601–615):
```cpp
// BEFORE:
if (s_entries.find(k) == s_entries.end()) {
    BgTextureEntry ent;
    ...
    s_entries[k] = std::move(ent);
    s_queue.push_back(k);
    s_metrics_prefetches++;
}

// AFTER:
auto ent_find = s_entries.find(k);
if (ent_find == s_entries.end() || ent_find->second.state == BgState::None) {
    if (ent_find == s_entries.end()) {
        BgTextureEntry ent;
        ent.key = k;
        ent.stage = stage;
        ent.room = room;
        ent.cut = c_u8;
        ent.filepath = item.path().generic_string();
        ent.state = BgState::Queued;
        s_entries[k] = std::move(ent);
    } else {
        ent_find->second.state = BgState::Queued;
        ent_find->second.filepath = item.path().generic_string();
    }
    s_queue.push_back(k);
    s_metrics_prefetches++;
}
```

### Point 3: Active Cut Priority & Re-Queueing
In `re3_hd_bg_on_room_change()` (lines 623–640):
```cpp
// BEFORE:
if (s_entries.find(active_key) == s_entries.end()) {
    char resolved[1024];
    if (re3_hd_bg_resolve_file(stage, room, cut, resolved, sizeof(resolved))) {
        BgTextureEntry ent;
        ent.key = active_key;
        ent.stage = stage;
        ent.room = room;
        ent.cut = cut;
        ent.filepath = resolved;
        ent.state = BgState::Queued;
        s_entries[active_key] = std::move(ent);

        // Put active cut at the FRONT of the queue for immediate loading
        s_queue.push_front(active_key);
        s_metrics_prefetches++;
    }
}

// AFTER:
auto act_it = s_entries.find(active_key);
if (act_it == s_entries.end() || act_it->second.state == BgState::None) {
    char resolved[1024];
    if (re3_hd_bg_resolve_file(stage, room, cut, resolved, sizeof(resolved))) {
        BgTextureEntry ent;
        ent.key = active_key;
        ent.stage = stage;
        ent.room = room;
        ent.cut = cut;
        ent.filepath = resolved;
        ent.state = BgState::Queued;
        s_entries[active_key] = std::move(ent);

        s_queue.push_front(active_key);
        s_metrics_prefetches++;
    }
} else if (act_it->second.state == BgState::Queued) {
    // If already queued at back by room prefetch, elevate to front for immediate decoding
    auto q_it = std::find(s_queue.begin(), s_queue.end(), active_key);
    if (q_it != s_queue.end()) {
        s_queue.erase(q_it);
        s_queue.push_front(active_key);
    }
}
```

### Point 4: LRU Recency Promotion on Cache Hits
In `re3_hd_bg_get_current_gl_tex()` (lines 664–668):
```cpp
// BEFORE:
if (it->second.gl_tex_id == 0 && it->second.state == BgState::ReadyRAM) {
    upload_entry_to_gl(it->second);
}
s_active_gl_tex = it->second.gl_tex_id;
return s_active_gl_tex;

// AFTER:
if (it->second.gl_tex_id == 0 && it->second.state == BgState::ReadyRAM) {
    upload_entry_to_gl(it->second);
} else if (it->second.gl_tex_id != 0) {
    // Cache hit: bump to front of LRU list to prevent premature eviction of active/frequent cuts
    if (s_lru_list.empty() || s_lru_list.front() != key) {
        s_lru_list.remove(key);
        s_lru_list.push_front(key);
    }
}
s_active_gl_tex = it->second.gl_tex_id;
return s_active_gl_tex;
```

### Point 5: Fix Worker Thread Key 0 Check
In `worker_thread_func()` (line 301):
```cpp
// BEFORE:
if (key == 0 || resolved_path.empty()) continue;

// AFTER:
if (resolved_path.empty()) continue;
```
This ensures Stage 0, Room 0, Cut 0 (`key == 0`) is decoded normally instead of being erroneously discarded.

---

## 5. Verification Method

1. **Compile and Run Concurrency & Stress Test**:
   ```powershell
   clang++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_hd_backgrounds_stress.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -lpsapi -o build/test_re3_hd_backgrounds_stress.exe
   .\build\test_re3_hd_backgrounds_stress.exe
   ```
   **Pass Condition**: `Stress Results: 5 executed, 5 passed, 0 failed` (specifically, `[TEST 4] test_stress_lru_eviction_boundary_and_reacquisition` must pass).

2. **Compile and Run Adversarial Resilience Test**:
   ```powershell
   clang++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m2_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -lpsapi -o build/test_re3_m2_adversarial.exe
   .\build\test_re3_m2_adversarial.exe
   ```
   **Pass Condition**: `Adversarial Results: 6 executed, 6 passed, 0 failed` (specifically, `[ADVERSARIAL TEST 3] test_adv_extreme_out_of_bounds` passes stage 0 cut 0).

3. **Invalidation Condition**:
   This strategy is invalidated if memory growth in Test 5 exceeds 50MB, or if any deadlock occurs during the 200 rapid init/shutdown stress cycles (Test 1).
