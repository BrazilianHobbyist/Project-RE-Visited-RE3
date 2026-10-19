# Handoff Report: Key=0 Sentinel Fix Strategy in RE3 HD Backgrounds

**Agent**: Explorer M2 R2 2 (`teamwork_preview_explorer`)  
**Working Directory**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_r2_2\`  
**Milestone**: M2 R2  
**Target File**: `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`  

---

## 1. Observation

### 1.1 Direct Source Code Inspection
In `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`:

1. **Background key generation (`lines 40-44`)**:
   ```cpp
   inline uint32_t bg_make_key(uint8_t stage, uint8_t room, uint8_t cut) {
       return (static_cast<uint32_t>(stage) << 16) |
              (static_cast<uint32_t>(room) << 8) |
              static_cast<uint32_t>(cut);
   }
   ```
   For `stage = 0, room = 0, cut = 0` (Resident Evil 3's prologue / uptown / debug boundary room angle `R000_00.png`):
   `bg_make_key(0, 0, 0) == (0 << 16) | (0 << 8) | 0 == 0`.
   The key domain spans 24 bits (`0x00000000` to `0x00FFFFFF`). Key `0` is a valid, canonical background identifier.

2. **Defective worker loop in `worker_thread_func()` (`lines 274-321`)**:
   ```cpp
   while (!s_stop) {
       uint32_t key = 0;
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
                   it->second.state = BgState::Loading;
                   resolved_path = it->second.filepath;
               }
           }
       }

       if (key == 0 || resolved_path.empty()) continue; // <-- LINE 301 DEFECT

       DecodedImage img;
       bool success = decode_png_file(resolved_path, img);
       ...
   ```

3. **Behavior upon popping key `0`**:
   - `key` is popped as `0`.
   - `s_entries.find(0)` succeeds, finding the queued `R000_00.png` entry.
   - `it->second.state` transitions from `BgState::Queued` to `BgState::Loading`.
   - `resolved_path` is populated with the valid absolute/relative path to `R000_00.png`.
   - Critical section unlocks.
   - Line 301 evaluates: `if (key == 0 || resolved_path.empty()) continue;`.
   - Because `key == 0` evaluates to `true`, `continue` executes immediately.
   - `decode_png_file()` is completely bypassed.
   - The entry in `s_entries[0]` is permanently stranded in `BgState::Loading`.
   - Subsequent calls to `re3_hd_bg_has_hd_current()` check `(it->second.gl_tex_id != 0 || it->second.state == BgState::ReadyRAM)` and return `false`.
   - Furthermore, because `s_entries.find(0)` already contains key 0, `re3_hd_bg_on_room_change(0, 0, 0)` will never re-queue it, permanently locking Cut 0 of Stage 0 / Room 0 out of HD backgrounds for the remainder of the session.

### 1.2 Empirical Reproduction
Command executed:
```powershell
c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m2_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_m2_adversarial.exe ; .\build\test_re3_m2_adversarial.exe
```
Verbatim test failure output:
```
[ADVERSARIAL TEST 3] test_adv_extreme_out_of_bounds...
psxrecomp: [Textures] Active pack: "OobPack" at "C:/Users/victo/AppData/Local/Temp/re3_oob_19169_15724/mods/textures/OobPack" [BG:ON, Models:ON, UI:ON, Dump:OFF]
    DEBUG: res0 = C:/Users/victo/AppData/Local/Temp/re3_oob_19169_15724/mods/textures/OobPack/backgrounds/R000_00.png
    DEBUG: is_pack_valid = 1, hd_backgrounds = 1
    Observation: stage 0, room 0, cut 1 (key = 1): wait_for_decode = 1, has_hd = 1 [SUCCESS]
    Observation: stage 0, room 0, cut 0 (key = 0): wait_for_decode = 0, has_hd = 0 [FAILURE]
    [BUG CONFIRMED] re3_hd_backgrounds.cpp:301 skips decoding when key == 0 (stage 0, room 0, cut 0).
  [FAIL] Stage 0, room 0, cut 0 must load (Failed: key == 0 discarded by worker thread) (Line 273: ok_d == true)
===============================================================
 Adversarial Results: 6 executed, 5 passed, 1 failed
===============================================================
```

---

## 2. Logic Chain

1. From Observation 1.1, the game engine packs `(stage, room, cut)` into a 32-bit integer where `key = (stage << 16) | (room << 8) | cut`.
2. From Observation 1.1, `stage = 0, room = 0, cut = 0` produces mathematical value `0`.
3. From Observation 1.1, `re3_hd_bg_resolve_file()` successfully discovers and resolves candidate `R000_00.png` on disk for `(0, 0, 0)`.
4. From Observation 1.1, `re3_hd_bg_on_room_change(0, 0, 0)` enqueues key `0` into `s_queue` and inserts an entry with key `0` into `s_entries`.
5. From Observation 1.1, `worker_thread_func()` dequeues `key = 0`, sets `s_entries[0].state = BgState::Loading`, and stores `resolved_path`.
6. From Observation 1.1 & 1.2, line 301 checks `if (key == 0 || resolved_path.empty()) continue;`.
7. Because `key == 0` is true, the condition triggers a loop skip (`continue;`), aborting `decode_png_file()`.
8. The entry `s_entries[0]` remains stranded in `BgState::Loading` indefinitely, causing `re3_hd_bg_has_hd_current()` to return `false` and `re3_hd_bg_render_current()` to fall back to low-resolution PS1 VRAM rendering.
9. Therefore, using `0` as an empty-job sentinel is defective and must be replaced by a robust, non-colliding mechanism.

---

## 3. Caveats

1. **Other Call Sites**: Analysis confirmed that no other function in `re3_hd_backgrounds.cpp` treats `key == 0` as invalid:
   - `s_entries.find(0)` correctly finds the map entry.
   - `s_lru_list.remove(0)` and `push_front(0)` correctly manage LRU ordering for key 0.
   - LRU eviction correctly protects active cut 0 via `victim_key == bg_make_key(s_active_stage, s_active_room, s_active_cut)`.
2. **Read-Only Constraint**: As instructed, no source code in `psxrecomp/` has been modified by this agent. The fix strategy formulated below is ready for implementation by the designated modifier agent.

---

## 4. Conclusion & Strategy Formulation

### 4.1 Evaluation of Fix Strategies

| Strategy | Mechanism | Pros | Cons | Recommendation |
|---|---|---|---|---|
| **Strategy 1: `bool has_job` flag** | Declare `bool has_job = false;` in worker loop; set to `true` only when a valid item is popped from `s_queue` and found in `s_entries` with a non-empty path. | Completely decouples queue dispatch logic from key value representation; zero sentinel collision risk. | Adds a 1-byte stack boolean. | **Primary Recommended Pattern** |
| **Strategy 2: Out-of-Domain Sentinel (`0xFFFFFFFF`)** | Initialize `key = 0xFFFFFFFFu` (`kInvalidBgKey`). Guard with `if (key == kInvalidBgKey \|\| resolved_path.empty()) continue;`. | Mathematically safe: `bg_make_key` only occupies bits 0..23 (`0x00FFFFFF` max). Bits 24..31 are strictly zero. Minimal diff. | Still relies on a magic sentinel number convention. | **Strong Defense-in-Depth Complement** |
| **Strategy 3: Solely `resolved_path.empty()`** | Rely only on `if (resolved_path.empty()) continue;` since `resolved_path` is only assigned when a valid entry is found. | Simplest guard check. | Leaves `key` initialized to 0 in outer scope, creating ambiguity if logged or debugged. | **Insufficient on its own** |

### 4.2 Recommended Synthesis: Defense in Depth (Strategy 1 + Strategy 2)

The most robust, clean, and idiomatic C++17 architecture combines:
1. Defining `constexpr uint32_t kInvalidBgKey = 0xFFFFFFFFu;` in the anonymous namespace.
2. Initializing `uint32_t key = kInvalidBgKey;` and `bool has_job = false;` at the start of each worker loop iteration.
3. In `worker_thread_func()` inside the critical section:
   - If `s_queue` is non-empty, pop `key = s_queue.front(); s_queue.pop_front();`.
   - Find in `s_entries`. If found:
     - If state is already `ReadyGL` or `ReadyRAM`: execute `continue;` (lock released by RAII).
     - If `it->second.filepath.empty()`: transition `it->second.state = BgState::Failed;` and `continue;` (avoids stranding entries in `Loading`).
     - Otherwise: set `it->second.state = BgState::Loading; resolved_path = it->second.filepath; has_job = true;`.
4. Outside the critical section:
   - Guard with: `if (!has_job || resolved_path.empty()) continue;`.
   - Decode PNG file.
   - On success: set `staging_pixels` and `state = BgState::ReadyRAM`.
   - On failure: set `state = BgState::Failed`.

### 4.3 Exact Proposed Code Changes for Implementer

Target file: `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`

#### Change A: Add Sentinel Constant & Struct Default (Around line 39-65)
```cpp
// BEFORE (line 38-44):
constexpr size_t kMaxCachedTextures = 32;

inline uint32_t bg_make_key(uint8_t stage, uint8_t room, uint8_t cut) {
    return (static_cast<uint32_t>(stage) << 16) |
           (static_cast<uint32_t>(room) << 8) |
           static_cast<uint32_t>(cut);
}

// AFTER:
constexpr size_t kMaxCachedTextures = 32;
constexpr uint32_t kInvalidBgKey = 0xFFFFFFFFu;

inline uint32_t bg_make_key(uint8_t stage, uint8_t room, uint8_t cut) {
    return (static_cast<uint32_t>(stage) << 16) |
           (static_cast<uint32_t>(room) << 8) |
           static_cast<uint32_t>(cut);
}
```
And in `struct BgTextureEntry` (line 61):
```cpp
// BEFORE:
struct BgTextureEntry {
    uint32_t key = 0;
...

// AFTER:
struct BgTextureEntry {
    uint32_t key = kInvalidBgKey;
...
```

#### Change B: Refactor `worker_thread_func()` (Lines 274-302)
```cpp
// BEFORE (lines 274-302):
    while (!s_stop) {
        uint32_t key = 0;
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
                    it->second.state = BgState::Loading;
                    resolved_path = it->second.filepath;
                }
            }
        }

        if (key == 0 || resolved_path.empty()) continue;

// AFTER:
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

### 4.4 End-to-End Call Graph Trace with the Fix

```
re3_hd_bg_on_room_change(0, 0, 0)
 │
 ├── stage=0, room=0, cut=0 -> active_key = bg_make_key(0, 0, 0) == 0x00000000
 ├── re3_hd_bg_resolve_file(0, 0, 0) -> finds "mods/textures/<pack>/backgrounds/R000_00.png"
 ├── s_entries[0] = { key: 0, filepath: "...", state: Queued }
 ├── s_queue.push_front(0)
 └── s_cv.notify_one()
       │
       ▼
 worker_thread_func() (Worker Thread)
  ├── has_job = false, key = 0xFFFFFFFF
  ├── s_cv.wait wakes up with lock
  ├── key = s_queue.front() (key becomes 0)
  ├── s_queue.pop_front()
  ├── it = s_entries.find(0) -> FOUND
  ├── it->second.state = BgState::Loading
  ├── resolved_path = "R000_00.png"
  ├── has_job = true
  ├── lock unlocks
  ├── if (!has_job || resolved_path.empty()) -> FALSE (proceeds to decode!)
  ├── decode_png_file("R000_00.png", img) -> SUCCEEDS (RGBA pixels loaded)
  ├── lock reacquired
  ├── it->second.staging_pixels = img
  ├── it->second.state = BgState::ReadyRAM
  └── lock unlocks
        │
        ▼
 re3_hd_bg_has_hd_current() (Main Thread)
  ├── key = bg_make_key(0, 0, 0) == 0
  ├── it = s_entries.find(0)
  └── returns (gl_tex_id != 0 || state == ReadyRAM) -> TRUE! [FIX CONFIRMED]
        │
        ▼
 re3_hd_bg_get_current_gl_tex() (Main Thread)
  ├── calls upload_entry_to_gl() -> generates GLuint, uploads RGBA, state = ReadyGL
  └── returns valid GL texture handle
        │
        ▼
 re3_hd_bg_render_current(s_hr_fbo, width, height)
  └── Blits 1440p/4K background quad into s_hr_fbo with pixel-perfect mask alignment
```

---

## 5. Verification Method

To verify the resolution after the implementer applies the change:

1. **Recompile and Run the Native Adversarial Test Binary**:
   ```powershell
   c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m2_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_m2_adversarial.exe
   .\build\test_re3_m2_adversarial.exe
   ```
   **Expected Outcome**:
   - `[ADVERSARIAL TEST 1] test_adv_malformed_png_injection... [PASS]`
   - `[ADVERSARIAL TEST 2] test_adv_path_traversal_injection... [PASS]`
   - `[ADVERSARIAL TEST 3] test_adv_extreme_out_of_bounds... [PASS]`
     - Line output: `Observation: stage 0, room 0, cut 0 (key = 0): wait_for_decode = 1, has_hd = 1 [SUCCESS]`
   - `[ADVERSARIAL TEST 4] test_adv_reshdp_variations_and_precedence... [PASS]`
   - `[ADVERSARIAL TEST 5] test_adv_partial_packs_transparent_fallback... [PASS]`
   - `[ADVERSARIAL TEST 6] test_adv_cache_thrashing_and_concurrency... [PASS]`
   - Summary: `Adversarial Results: 6 executed, 6 passed, 0 failed`
   - Exit code: `0`.

2. **Run Native Background Unit Test Suite**:
   ```powershell
   .\build\test_re3_hd_backgrounds_native.exe
   ```
   **Expected Outcome**: 5 passed, 0 failed.

3. **Run Native Background Concurrency Stress Test Suite**:
   ```powershell
   .\build\test_re3_hd_backgrounds_stress.exe
   ```
   **Expected Outcome**: 5 passed, 0 failed.

4. **Invalidation Condition**:
   If `[ADVERSARIAL TEST 3]` continues to fail on line 273 or reports `wait_for_decode = 0` for `stage 0, room 0, cut 0`, the fix is invalid.
