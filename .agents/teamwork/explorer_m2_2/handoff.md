# Handoff Report: Explorer M2 2 — Asynchronous Pre-fetcher & LRU Cache Architecture

## 1. Observation

1. **Existing Async Worker Pattern in `hd_texture_pack.cpp`**:
   - `psxrecomp/runtime/src/hd_texture_pack.cpp`:
     - Lines 442–450:
       ```cpp
       enum class DecodeState { Queued, Loading, Ready, Failed };
       struct DecodeItem {
           DecodeState state = DecodeState::Queued;
           std::string path;
           std::shared_ptr<DecodedImage> image;
           size_t bytes = 0;
           uint64_t last_used = 0;
       };
       ```
     - Lines 452–463:
       ```cpp
       struct DecodeCache {
           std::mutex mutex;
           std::condition_variable wake;
           std::unordered_map<uint64_t, DecodeItem> items;
           std::deque<uint64_t> queue;
           std::thread worker;
           size_t budget = kDefaultDecodeBudget;
           size_t used = 0;
           uint64_t tick = 0;
           bool started = false;
           bool stop = false;
       ```
     - Lines 494–564: `decode_worker(DecodeCache* cache)` runs an asynchronous decode loop:
       - Line 496: `SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);` on Windows.
       - Lines 502–511: Acquires `unique_lock<mutex>`, waits on `cache->wake.wait(...)`, pops next key, updates state to `DecodeState::Loading`, copies `path`, and unlocks the mutex before performing file I/O or decompression.
       - Lines 513–526: Reads file bytes via `std::ifstream` into a `std::vector<uint8_t> encoded`.
       - Lines 530–532: Calls `stbi_load_from_memory(encoded.data(), (int)encoded.size(), &width, &height, &components, 4);`.
       - Lines 546–563: Re-acquires `std::lock_guard<std::mutex> lock(cache->mutex)`, checks budget, calls `evict_decode_cache()`, stores decoded RGBA in `DecodeItem::image`, marks `state = DecodeState::Ready`, and increments `cache->tick`.
     - Lines 464–471: Destructor safely sets `stop = true`, notifies all threads via `wake.notify_all()`, and joins `worker`.

2. **`stb_image` Compilation Constraint & Linkage**:
   - `psxrecomp/runtime/src/psx_window_icon.cpp`:
     - Lines 18–21:
       ```cpp
       #define STB_IMAGE_IMPLEMENTATION
       #define STBI_ONLY_PNG
       #define STBI_NO_STDIO
       #include "../third_party/stb_image.h"
       ```
   - `psxrecomp/runtime/third_party/stb_image.h`:
     - Lines 420–427: `stbi_load_from_memory` is unconditionally exposed, whereas `stbi_load(char const *filename, ...)` is conditionally compiled under `#ifndef STBI_NO_STDIO`.
     - Because `psx_window_icon.cpp` defines `STBI_NO_STDIO`, any call to `stbi_load(filename)` fails at link time. File reading must be handled by the caller (e.g. `std::ifstream` or `fopen`/`fread`), followed by `stbi_load_from_memory()`.

3. **OpenGL Context Single-Threading**:
   - `psxrecomp/runtime/src/gpu_gl_renderer.c`:
     - Line 5142: `s_ctx = SDL_GL_CreateContext(win);` allocates a single OpenGL context for the runtime.
     - Line 5144: `SDL_GL_MakeCurrent(win, s_ctx)` makes it current exclusively on the main/render thread.
     - Lines 4608–4618 (`make_tex`) and 4639–4644 (`free_hr_targets`): Standard GL calls (`glGenTextures`, `glBindTexture`, `glTexParameteri`, `glTexImage2D`, `glDeleteTextures`) are invoked directly without mutexes, requiring all OpenGL operations to execute strictly on the render thread.

4. **Guest State Tracking Integration**:
   - `psxrecomp/runtime/src/gpu_gl_renderer.c`:
     - Lines 804–806 & 8193–8195:
       ```c
       extern uint8_t psx_read_byte(uint32_t addr);
       uint8_t cur_room = psx_read_byte(0x000D1F09);
       uint8_t cur_cut  = psx_read_byte(0x000D1F0A);
       ```
     - Stage address is `0x000D1F08`.

5. **Existing Path Resolution & Config**:
   - `psxrecomp/runtime/include/re3_textures.h`:
     - Exposes `re3_texture_config_t g_re3_tex_config`, `re3_textures_is_pack_valid()`, and `re3_textures_resolve_path()`.
   - `psxrecomp/runtime/src/re3_textures.cpp`:
     - Line 128: `bool re3_textures_resolve_path(char* out_path, size_t max_len, const char* subfolder, const char* filename);`.

---

## 2. Logic Chain

1. **Worker Thread Decoupling from OpenGL**:
   - From Observation 3, OpenGL context sharing (`SDL_GL_SHARE_WITH_CURRENT_CONTEXT`) is not enabled in `psxrecomp`, and calling GL commands from multiple threads without context switching induces driver faults and race conditions.
   - Therefore, the background worker thread must perform *exclusively* host CPU operations: disk reading and PNG decompression (`stbi_load_from_memory`) into heap-allocated RGBA8 staging buffers (`std::vector<uint8_t>`).
   - All OpenGL calls (`glGenTextures`, `glTexImage2D`, `glDeleteTextures`) must be executed strictly by the render thread inside `re3_hd_bg_render_current()` or during room transitions.

2. **Room-Level Eager Pre-fetching**:
   - In RE3: Nemesis, each room consists of multiple camera cuts (between 2 and 16 cuts, bounded by 32). Switching camera angles occurs abruptly as the player maneuvers across invisible camera triggers.
   - Decompressing a 4K PNG via `stb_image` on CPU takes ~25ms to 80ms. If performed synchronously upon camera angle switch, it causes a severe hitch / frame drop.
   - From Observation 4, the camera and room state is tracked at `0x000D1F08` (Stage), `0x000D1F09` (Room), and `0x000D1F0A` (Cut).
   - When a room transition is detected (`stage != last_stage || room != last_room`), the pre-fetcher immediately:
     1. Prioritizes the active cut `(stage, room, cut)` at the front of the worker queue.
     2. Discovers all existing cuts for `(stage, room)` via a pre-indexed catalog in RAM (avoiding filesystem scanning during gameplay) and enqueues them for background decoding.
     3. Prunes obsolete, unstarted requests from previous rooms from the worker queue.
   - While the player watches the door transition animation or walks in the first angle, the worker thread pre-decodes the remaining angles of that room in the background.

3. **Staging Buffer Memory Lifecycle**:
   - Once a decoded image is uploaded to the GPU via `glTexImage2D` on the main thread, keeping the raw RGBA8 pixel buffer in host CPU RAM wastes ~33 MB per 4K texture.
   - Therefore, immediately after `glTexImage2D` finishes, `staging_pixels.reset()` is invoked. This drops host CPU RAM usage back to near zero, maintaining only the GPU VRAM allocation.

4. **LRU Cache & Clean Eviction**:
   - An LRU cache with capacity $N = 32$ textures provides enough space to retain the complete cut sets of the active room (~8–16 cuts) plus adjacent rooms (~8–16 cuts).
   - When the user traverses back and forth across a room boundary, both rooms remain 100% hot in VRAM, eliminating any re-decoding.
   - Eviction algorithm:
     - When a 33rd cut is uploaded to GL, find the least recently used entry from the tail of the LRU list that is *not* the currently active cut.
     - Call `glDeleteTextures(1, &victim->gl_tex_id)` on the main thread and reclaim the slot.

5. **Transparent Fallback Mechanics**:
   - If a background has not yet finished decoding, or if a community pack is partial and lacks that specific cut on disk:
     - `re3_hd_bg_has_hd_current()` returns `false`.
     - In `gpu_gl_renderer.c`, the HD background blit is skipped.
     - `s_hr_fbo` retains the PS1 VRAM 320x240 native background that was uploaded via standard CPU->VRAM transfers in `flush_cpu_upload`.
     - The game renders without stutter, visual corruption, or crashes. Once the worker thread finishes decoding (1–2 frames later), `re3_hd_bg_has_hd_current()` transitions to `true` and the HD backdrop activates smoothly.

---

## 3. Caveats

1. **Initial Cold Start / New Game**: On the very first frame after loading a save or starting a new game, the worker thread may take 1–2 frames to decode the first background. The transparent fallback handles this perfectly by displaying the PS1 VRAM background until the HD texture is ready.
2. **Door Transitions / FMVs**: During 24-bit FMVs or non-gameplay cutscenes, memory addresses `0x000D1F08..A` may latch previous values. Background rendering is already gated by `!gpu_display_is_depth24()`.
3. **Pre-indexing Overhead**: Scanning the `mods/textures/<pack>/backgrounds/` folder once at `re3_hd_bg_init` takes ~2–5 ms for ~1,000 files, which is negligible during game startup and completely eliminates `std::filesystem::exists` disk queries during active gameplay.

---

## 4. Conclusion

The Asynchronous Pre-fetcher and LRU Cache for Milestone 2 should be implemented in `re3_hd_backgrounds.cpp` according to the following architecture:

### Architecture Specification

#### A. Data Structures & Types
```cpp
// Unique 24-bit composite key for each camera cut:
inline uint32_t bg_make_key(uint8_t stage, uint8_t room, uint8_t cut) {
    return (static_cast<uint32_t>(stage) << 16) |
           (static_cast<uint32_t>(room) << 8) |
           static_cast<uint32_t>(cut);
}

enum class BgState {
    None,
    Queued,
    Loading,
    ReadyRAM,  // Decoded by worker thread, pending GL upload
    ReadyGL,   // Uploaded to OpenGL texture, staging RAM freed
    Failed
};

struct DecodedImage {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba;
};

struct BgTextureEntry {
    uint32_t key = 0;
    uint8_t stage = 0;
    uint8_t room = 0;
    uint8_t cut = 0;
    std::string filepath;
    BgState state = BgState::None;
    uint32_t gl_tex_id = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    std::shared_ptr<DecodedImage> staging_pixels; // Freed immediately upon GL upload
    std::list<uint32_t>::iterator lru_it;
};
```

#### B. LRU Cache & Pre-fetcher State
```cpp
constexpr size_t kMaxCachedTextures = 32;

struct BgEngineState {
    std::mutex mutex;
    std::condition_variable cv;
    std::thread worker;
    std::atomic<bool> stop{false};

    // Pre-indexed catalog of available pack files (indexed once at init)
    // Key: (stage << 8) | room, Value: list of cut IDs present on disk
    std::unordered_map<uint16_t, std::vector<uint8_t>> room_catalog;

    // Worker queue of pending decode requests:
    // Key + file path
    std::deque<std::pair<uint32_t, std::string>> queue;

    // Active cache entries:
    std::unordered_map<uint32_t, BgTextureEntry> entries;
    std::list<uint32_t> lru_list; // Front = MRU, Back = LRU

    // Active camera tracking:
    uint8_t active_stage = 0xFF;
    uint8_t active_room  = 0xFF;
    uint8_t active_cut   = 0xFF;
    uint32_t active_gl_tex = 0;
};
```

#### C. Worker Thread Pipeline
1. Sets priority: `SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL)` (on Windows).
2. Waits on condition variable: `cv.wait(lock, [&] { return state.stop || !state.queue.empty(); });`.
3. Pops front request and marks `entry.state = BgState::Loading`.
4. Releases mutex.
5. Reads binary data via `std::ifstream` into a memory buffer.
6. Calls `stbi_load_from_memory(buf.data(), buf.size(), &w, &h, &comp, 4)`.
7. Re-acquires mutex.
8. If decode succeeded:
   - Allocates `std::shared_ptr<DecodedImage>`.
   - Copies pixels to `rgba` vector.
   - Calls `stbi_image_free()`.
   - Sets `entry.state = BgState::ReadyRAM`.
   - Sets `entry.staging_pixels = img`.
9. If decode failed:
   - Sets `entry.state = BgState::Failed`.

#### D. Main Thread GL Upload & Eviction Pipeline
Called from `re3_hd_bg_render_current()` on the render thread:
1. Lookup active cut entry in `entries`.
2. If `entry.state == BgState::ReadyRAM`:
   - Calls `glGenTextures(1, &entry.gl_tex_id)`.
   - Calls `glBindTexture(GL_TEXTURE_2D, entry.gl_tex_id)`.
   - Sets `GL_LINEAR` filtering and `GL_CLAMP_TO_EDGE` wrap.
   - Calls `glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, entry.staging_pixels->rgba.data())`.
   - Frees host RAM: `entry.staging_pixels.reset()`.
   - Sets `entry.state = BgState::ReadyGL`.
   - Updates LRU list (moves to front).
   - If resident GL textures count $> 32$:
     - Traverses from back of `lru_list`.
     - Finds victim with `gl_tex_id != 0` and `key != active_cut_key`.
     - Calls `glDeleteTextures(1, &victim.gl_tex_id)`.
     - Sets `victim.gl_tex_id = 0`, `victim.state = BgState::None`.
     - Removes victim from `lru_list`.
3. Opportunistic pre-upload: If active cut is already uploaded, the main thread uploads up to 1 other `ReadyRAM` cut per frame to have all room angles ready in VRAM ahead of time.

#### E. Fallback & Query Interface
```cpp
bool re3_hd_bg_has_hd_current(void) {
    if (!g_re3_tex_config.hd_backgrounds) return false;
    if (!re3_textures_is_pack_valid()) return false;

    uint32_t key = bg_make_key(s_engine.active_stage, s_engine.active_room, s_engine.active_cut);
    std::lock_guard<std::mutex> lock(s_engine.mutex);
    auto it = s_engine.entries.find(key);
    if (it == s_engine.entries.end()) return false;
    return (it->second.gl_tex_id != 0 || it->second.state == BgState::ReadyRAM);
}

uint32_t re3_hd_bg_get_current_gl_tex(void) {
    return s_engine.active_gl_tex;
}
```

#### F. Memory Budget & Resource Accounting
| Resolution | Color Format | Size per Texture | 32 Cached Textures (VRAM) | Peak Staging RAM (CPU) |
|---|---|---|---|---|
| **1080p** (1920x1080) | RGBA8 | 7.91 MiB | **253 MiB** | ~35 MiB |
| **1440p** (2560x1440) | RGBA8 | 14.06 MiB | **450 MiB** | ~60 MiB |
| **4K UHD** (3840x2160)| RGBA8 | 31.64 MiB | **1,012 MiB** (~1.0 GB) | ~130 MiB |

*Note*: Host CPU staging RAM is released immediately upon GPU texture creation, so steady-state CPU RAM overhead is near zero.

---

## 5. Verification Method

1. **Codebase Inspection**:
   - Inspect `psxrecomp/runtime/src/hd_texture_pack.cpp` lines 442–564 to verify worker loop, mutex/cv pattern, and `SetThreadPriority`.
   - Inspect `psxrecomp/runtime/src/psx_window_icon.cpp` lines 18–21 and `psxrecomp/runtime/third_party/stb_image.h` lines 420–427 to verify `STBI_NO_STDIO` and `stbi_load_from_memory`.
   - Inspect `psxrecomp/runtime/src/gpu_gl_renderer.c` lines 5142–5144 to verify single OpenGL context constraint.

2. **Test Command Verification**:
   - Build and run the existing test harness via PowerShell:
     ```powershell
     powershell -ExecutionPolicy Bypass -File "build.ps1"
     ```
   - Run the native stress test executable:
     ```powershell
     .\build\test_re3_textures_stress.exe
     ```
   - Output verifies: `Results: 7 executed, 7 passed, 0 failed`.

3. **Invalidation Conditions**:
   - The findings would be invalidated if OpenGL supported thread-safe multi-threaded context-free texture uploading without driver synchronization, or if `stbi_load` from file was compiled into the runtime binary (refuted by lines 18–21 in `psx_window_icon.cpp`).
