# Review and Adversarial Challenge Report: Milestone 2 Remediation (Round 2)

**Reviewer Agent**: `reviewer_m2_r2_1` (Roles: Reviewer, Critic)  
**Target File**: `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`  
**Worker Under Review**: Worker M2 2 (`worker_m2_2`)  
**Working Directory**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\reviewer_m2_r2_1\`  
**Milestone**: M2 Remediation Quality Review  

---

## Review Summary

- **Verdict**: **APPROVE**
- **Integrity Assessment**: **NO INTEGRITY VIOLATIONS DETECTED**. No hardcoded outputs, mock facades, fake logs, or shortcuts were found. All 4 remediation items are implemented with genuine, thread-safe, state-maintaining logic.
- **Empirical Gate Results**:
  1. Runtime Build (`build.ps1`): **PASS** (Exit Code 0)
  2. Native Unit Suite (`test_re3_hd_backgrounds_native.exe`): **5/5 PASS**
  3. Concurrency Stress Suite (`test_re3_hd_backgrounds_stress.exe`): **5/5 PASS**
  4. Adversarial Resilience Suite (`test_re3_m2_adversarial.exe`): **6/6 PASS**
  5. Master E2E Suite (`run_e2e_tests.py`): **155/155 PASS** across all 5 tiers

---

## 1. 5-Component Handoff Report

### 1.1 Observation

1. **Remediation Point 1: LRU Eviction Zombie State & Re-acquisition** (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp:359-376`):
   Direct code inspection reveals:
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
               ent_it->second.staging_pixels.reset();
               s_metrics_evictions++;
               s_lru_list.erase(std::next(it).base());
               s_entries.erase(ent_it);
               break;
           }
       }
   }
   ```
   The evicted texture is erased from `s_entries` (`s_entries.erase(ent_it)`), preventing zombie entries from blocking future re-acquisition.

2. **Remediation Point 2: Room Change Re-Queueing Logic** (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp:617-668`):
   - For room prefetch: checks `if (ent_find == s_entries.end() || ent_find->second.state == BgState::None)` (lines 618–634), correctly handling both erased entries and any uninitialized state.
   - For active cut: checks `if (act_it == s_entries.end() || act_it->second.state == BgState::None)` (lines 645–660), resolves the file path, creates a new entry with `BgState::Queued`, and executes `s_queue.push_front(active_key)`.
   - Priority elevation: if active cut was already in `s_queue` as `BgState::Queued` from prefetch (lines 661–668), it is relocated to the front:
     ```cpp
     auto q_it = std::find(s_queue.begin(), s_queue.end(), active_key);
     if (q_it != s_queue.end()) {
         s_queue.erase(q_it);
         s_queue.push_front(active_key);
     }
     ```

3. **Remediation Point 3: Key 0 Sentinel in Worker Thread** (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp:39, 276-308`):
   - Defined `constexpr uint32_t kInvalidBgKey = 0xFFFFFFFFu;` in anonymous namespace (line 39).
   - In `worker_thread_func()`:
     ```cpp
     bool has_job = false;
     uint32_t key = kInvalidBgKey;
     std::string resolved_path;
     ...
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
     ...
     if (!has_job || resolved_path.empty()) continue;
     ```
   - For Stage 0, Room 0, Cut 0 (`key = 0`), `has_job` evaluates to `true`, and line 308 does not drop the job. Decoding proceeds normally.

4. **Remediation Point 4: LRU Recency Promotion on Hits** (`psxrecomp/runtime/src/re3_hd_backgrounds.cpp:692-701`):
   In `re3_hd_bg_get_current_gl_tex()`:
   ```cpp
   if (it->second.gl_tex_id == 0 && it->second.state == BgState::ReadyRAM) {
       upload_entry_to_gl(it->second);
   } else if (it->second.gl_tex_id != 0) {
       if (!s_lru_list.empty() && s_lru_list.front() != key) {
           s_lru_list.remove(key);
           s_lru_list.push_front(key);
       }
   }
   s_active_gl_tex = it->second.gl_tex_id;
   return s_active_gl_tex;
   ```
   - An O(1) branch `s_lru_list.front() != key` avoids list search on steady-state frames.
   - When switching to another already-resident angle, `key` is moved to the head of `s_lru_list`, preventing premature eviction.

5. **Empirical Verification Results**:
   - `powershell -ExecutionPolicy Bypass -File "build.ps1"`:
     - Result: Code 0. Binary linked at `build/Resident_Evil_3__Nemesis_Recompiled.exe`.
   - `.\build\test_re3_hd_backgrounds_native.exe`:
     - Result: Code 0. `Results: 5 executed, 5 passed, 0 failed`.
   - `.\build\test_re3_hd_backgrounds_stress.exe`:
     - Result: Code 0. `Stress Results: 5 executed, 5 passed, 0 failed`.
     - Test 4 output:
       ```
       Cut 0 successfully uploaded with GL tex ID 1
       After 33 texture uploads: evictions=1
       Switching back to evicted Cut 0...
       Re-acquisition check: has_hd_current() = 1
       Re-acquisition check: get_current_gl_tex() = 1
       [PASS] test_stress_lru_eviction_boundary_and_reacquisition
       ```
   - `.\build\test_re3_m2_adversarial.exe`:
     - Result: Code 0. `Adversarial Results: 6 executed, 6 passed, 0 failed`.
     - Test 3 output:
       ```
       DEBUG: res0 = C:/Users/victo/AppData/Local/Temp/re3_oob_19169_15724/mods/textures/OobPack/backgrounds/R000_00.png
       Observation: stage 0, room 0, cut 1 (key = 1): wait_for_decode = 1, has_hd = 1 [SUCCESS]
       Observation: stage 0, room 0, cut 0 (key = 0): wait_for_decode = 1, has_hd = 1 [SUCCESS]
       [PASS] test_adv_extreme_out_of_bounds
       ```
   - `python tests/e2e/run_e2e_tests.py`:
     - Result: Code 0. `Total Tests: 155, Passed: 155, Failed: 0, Errors: 0, Skipped: 0` in 9.826s.

---

### 1.2 Logic Chain

1. From Observation 1, erasing the victim iterator from `s_entries` ensures `s_entries.find(victim_key)` returns `s_entries.end()`, eliminating the zombie state that prevented re-fetching in Round 1.
2. From Observation 2, `re3_hd_bg_on_room_change()` branches handle `s_entries.end()` by freshly populating a `BgTextureEntry` and enqueuing it, prioritizing the active cut at the front of `s_queue`.
3. From Observation 3, replacing `key == 0` check with `has_job` and `kInvalidBgKey` allows Stage 0, Room 0, Cut 0 to be dequeued, decoded by `stbi_load_from_memory`, and uploaded to OpenGL without dropped jobs.
4. From Observation 4, promoting hits via `s_lru_list.push_front(key)` guarantees true LRU ordering, preventing frequently accessed resident cuts from being selected as eviction victims.
5. From Observation 5, all unit, stress, adversarial, and master E2E test suites pass with 100% success rate under native compilation and execution.
6. Therefore, the remediation implemented by Worker M2 2 is complete, correct, and production-ready.

---

### 1.3 Caveats

1. **Host Memory on Abandoned Prefetches**:
   If a player runs through dozens of rooms without viewing all camera angles, un-uploaded staging cuts remain in `BgState::ReadyRAM` until overwritten or shutdown. As identified in Round 1, this does not cause unbounded leakage because each unique room/cut key has at most 1 entry in `s_entries`, but bounds on `ReadyRAM` count could be considered for low-spec targets in future milestones.
2. **Foreground Mask Shader Integration**:
   `re3_hd_bg_get_screen_uv()` is fully verified mathematically in native tests, and its OpenGL polygon shader integration remains planned for Milestone 3 (`TEX_FS` uniform `u_is_hd`).

---

### 1.4 Conclusion

**Verdict**: **APPROVE**  
Worker M2 2 has resolved all identified defects in `re3_hd_backgrounds.cpp`. The code satisfies all architectural specifications of `PROJECT.md`, complies with the integrity policy, maintains thread safety, and passes all empirical test suites.

---

### 1.5 Verification Method

To independently reproduce this verification:

1. **Build Runtime**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "build.ps1"
   ```
   *Expected outcome*: Exit Code 0, binary created at `build/Resident_Evil_3__Nemesis_Recompiled.exe`.

2. **Run Native Background Unit Tests**:
   ```powershell
   .\build\test_re3_hd_backgrounds_native.exe
   ```
   *Expected outcome*: `Results: 5 executed, 5 passed, 0 failed` (Exit Code 0).

3. **Run Concurrency Stress Suite**:
   ```powershell
   .\build\test_re3_hd_backgrounds_stress.exe
   ```
   *Expected outcome*: `Stress Results: 5 executed, 5 passed, 0 failed` (Exit Code 0).

4. **Run Adversarial Resilience Suite**:
   ```powershell
   .\build\test_re3_m2_adversarial.exe
   ```
   *Expected outcome*: `Adversarial Results: 6 executed, 6 passed, 0 failed` (Exit Code 0).

5. **Run Master E2E Suite**:
   ```powershell
   python tests/e2e/run_e2e_tests.py
   ```
   *Expected outcome*: `Total Tests: 155, Passed: 155, Failed: 0, Errors: 0, Skipped: 0` (Exit Code 0).

---

## 2. Adversarial Challenge & Stress-Test Matrix

| Challenge / Attack Angle | Mechanism / Attack Scenario | Actual System Behavior | Result |
|---|---|---|---|
| **Cache Eviction Zombie Lockout** | Fill cache to 33 cuts to force eviction of Cut 0, then switch camera back to Cut 0 | Victim entry purged via `s_entries.erase()`; room change re-resolves and enqueues cut; uploaded cleanly with valid GL tex ID | **PASS** |
| **Stage 0 Room 0 Cut 0 Boundary** | Query active background `(0, 0, 0)` with `key == 0` | `has_job = true` sentinel permits worker thread to decode and upload `R000_00.png` | **PASS** |
| **LRU Recency Promotion** | Repeatedly access resident texture among 32 cached textures | Hit promotes key to head of `s_lru_list`; subsequent evictions choose older cuts from tail | **PASS** |
| **Concurrent Room Changes** | 10 worker threads issuing 1,000 rapid room/cut changes while main thread queries GL state | Synchronized under `s_mutex`; zero crashes, zero deadlocks, metrics correctly aggregated | **PASS** |
| **Malformed PNG Injection** | 0-byte, 1-byte, 7-byte, corrupted IDAT, truncated IHDR, fake BMP/GIF headers, 65MB file | Gracefully rejected by header validation and `stbi_load_from_memory`; returns false; transparent fallback to PS1 VRAM | **PASS** |
| **Path Traversal Attack** | Pack paths containing `..`, `C:\Windows\System32`, UNC paths | Rejected by canonical boundary check in `re3_hd_bg_resolve_file`; returns nullptr | **PASS** |
| **Rapid Init/Shutdown Cycles** | 200 consecutive cycles of `init()` followed immediately by `shutdown()` | Background thread safely joined via atomic `s_stop` and `s_cv.notify_all()`; zero hangs | **PASS** |
| **Continuous 1,000 Cut Stream** | Streaming 1,000 room transitions | Process memory bounded (+24 KB working set change); staging memory released immediately after GL upload | **PASS** |
