# Handoff Report: Explorer M2 1 (State Tracking & RESHDP File Resolution)

## Executive Summary
This report delivers the complete architectural specification, interface definition, deterministic file resolution algorithm, and runtime call graph for **Milestone 2: HD Pre-rendered Backgrounds Engine** in RE3Recomp.

---

## 1. Observation

### 1.1 Memory State Tracking (Stage, Room, Cut)
- **Guest RAM Addresses**:
  - `0x000D1F08`: Stage ID (`uint8_t`), values `1..5` (1=Uptown, 2=Downtown, 3=Clock Tower, 4=Hospital/Park, 5=Dead Factory).
  - `0x000D1F09`: Room ID (`uint8_t`), e.g., `0x00`, `0x01`, `0x0A`, `0x14`, `0x25`.
  - `0x000D1F0A`: Cut ID (`uint8_t`), camera cut/angle index within room (`0x00..0xFF`).
- **Memory Subsystem (`psxrecomp/runtime/src/memory.c`)**:
  - Line 2215: `uint8_t psx_read_byte(uint32_t addr)`:
    ```c
    uint8_t psx_read_byte(uint32_t addr) {
        if (g_ls_mode == 2) return (uint8_t)ls_read_hook(addr, 1, 0u);
        if (g_ls_mode != 1 || s_ls_op_active || g_ls_suppress_record || g_dma_exec_depth > 0)
            return psx_read_byte_raw(addr);
        ...
    }
    ```
  - Line 2224: `psx_read_byte_raw(uint32_t addr)`:
    ```c
    if (addr >= 0xC0000000u) { g_kseg2_ignored_reads++; return 0; }
    uint32_t phys = psx_phys_addr(addr);
    if (phys < RAM_SIZE) {
        return ram[phys];
    }
    ```
  - Physical translation (`psxrecomp/runtime/include/psx_memory.h` line 60):
    ```c
    static inline uint32_t psx_ram_map_read(uint32_t phys) {
        phys &= 0x1FFFFFFFu;
        return phys < PSX_MAIN_RAM_WINDOW_BYTES ? (phys & g_psx_ram_mask) : phys;
    }
    ```
    All MIPS addresses (`0x000D1F08`, `0x800D1F08` KSEG0, `0xA00D1F08` KSEG1) map to `ram[0x000D1F08]` with zero overhead.
- **Existing Present Usage (`psxrecomp/runtime/src/gpu_gl_renderer.c`)**:
  - Lines 804–806 (inside `actor_interp_commit_new_frame`):
    ```c
    extern uint8_t psx_read_byte(uint32_t addr);
    uint8_t cur_room = psx_read_byte(0x000D1F09);
    uint8_t cur_cut  = psx_read_byte(0x000D1F0A);
    ```
  - Lines 8193–8195 (inside `gl_renderer_present_vram`):
    ```c
    extern uint8_t psx_read_byte(uint32_t addr);
    uint8_t cur_room = psx_read_byte(0x000D1F09);
    uint8_t cur_cut  = psx_read_byte(0x000D1F0A);
    ```
  - Adding `uint8_t cur_stage = psx_read_byte(0x000D1F08);` completes the exact camera state tuple `(stage, room, cut)`.

### 1.2 RESHDP File Naming Schemes & Directory Layout
- **Location**: `mods/textures/<pack>/backgrounds/` (derived via `re3_textures_get_pack_path()`).
- **Canonical RESHDP Format**:
  - `R<stage><room:02X>_<cut:02X>.png` (e.g., `R100_00.png`, `R100_01.png`, `R20A_03.png`, `R305_0F.png`).
- **Community & Edge-Case Variations** (verified in `tests/e2e/harness/res_resolver.py` and `tests/e2e/test_tier5_adversarial.py`):
  1. Single-digit cut without leading zero: `R<stage><room:02X>_<cut>.png` (e.g., `R100_0.png`, `R100_2.png`).
  2. Lowercase filenames: `r<stage><room:02x>_<cut:02x>.png` (e.g., `r100_00.png`, `r20a_03.png`).
  3. Lowercase single-digit cut: `r<stage><room:02x>_<cut>.png` (e.g., `r100_0.png`).
  4. Uppercase extension: `*.PNG`.
  5. Room prefetch pattern: `r<stage:01x><room:02x>_*.png`.
- **Security & Adversarial Boundaries**:
  - Path traversal checks (`../../../../windows/system32`, `C:\Windows\System32`): must be rejected immediately, returning `NULL` for fallback.
  - Slashes normalization: both forward `/` and backward `\` slashes must be handled cleanly.
  - Unicode pack paths (e.g. `バイオハザード3_HD`): must be supported via UTF-8 path handling.

### 1.3 High-Resolution Framebuffer & Mask Alignment
- **Framebuffer Architecture (`gpu_gl_renderer.c`)**:
  - Line 568: `static GLuint s_hr_tex = 0, s_hr_fbo = 0, s_hr_rb = 0;`
  - Scale factor: `s_hr_scale` (typically 4x for 4K SSAA, or 2x..8x).
  - High-res buffer dimensions: `1024 * s_hr_scale` by `512 * s_hr_scale`.
  - Active display backbuffers in RE3:
    - Backbuffer 0: `(0, 0, 320, 240)` -> in `s_hr_fbo`: `(0, 0, 320 * S, 240 * S)`.
    - Backbuffer 1: `(0, 240, 320, 240)` -> in `s_hr_fbo`: `(0, 240 * S, 320 * S, 240 * S)`.
- **Occlusion Mask Quads**:
  - Screen dimensions are 320x240.
  - Screen-space UV formulas for any occlusion quad at `(x, y, w, h)`:
    - `u0 = (float)x / 320.0f`, `v0 = (float)y / 240.0f`
    - `u1 = (float)(x + w) / 320.0f`, `v1 = (float)(y + h) / 240.0f`
  - Drawing the mask quad sampling the HD texture at these UVs aligns 1:1 with the HD backdrop with subpixel precision.

### 1.4 Asynchronous Worker Infrastructure
- Existing reference pattern in `psxrecomp/runtime/src/hd_texture_pack.cpp` (lines 452–564):
  - Uses `std::mutex`, `std::condition_variable`, `std::deque`, and `std::thread`.
  - PNG decoding uses shared `stbi_load_from_memory` declared in `psxrecomp/runtime/third_party/stb_image.h` (implementation already compiled in `psx_window_icon.cpp`).
  - Worker thread priority set to below normal on Windows via `SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL)`.

---

## 2. Logic Chain

1. **State Tracking**:
   - `gpu_gl_renderer.c` already reads `psx_read_byte(0x000D1F09)` and `psx_read_byte(0x000D1F0A)` on every frame present.
   - By adding `psx_read_byte(0x000D1F08)` into the frame present handler, we obtain `(stage, room, cut)` at zero performance overhead.
   - When `(stage, room, cut)` differs from the previous latched state, `re3_hd_bg_on_room_change(stage, room, cut)` is triggered.

2. **Deterministic File Resolution**:
   - To eliminate filesystem thrashing, the resolver evaluates candidate filenames in strict descending order of likelihood:
     1. `R%1X%02X_%02X.png` (Standard uppercase hex RESHDP)
     2. `R%1X%02X_%u.png` (Single-digit cut uppercase hex)
     3. `r%1x%02x_%02x.png` (Standard lowercase hex)
     4. `r%1x%02x_%u.png` (Single-digit cut lowercase)
     5. `.PNG` uppercase extensions of 1–4.
   - If direct lookup fails, a case-insensitive directory cache maps lower-case names to actual disk filenames.
   - If all lookups fail or pack is disabled, return `NULL` to trigger transparent fallback to native VRAM.

3. **Eager Asynchronous Pre-fetching & LRU Cache**:
   - When entering a new room (`cur_room != prev_room` or `cur_stage != prev_stage`), the engine enumerates all cut files for `(stage, room)` via filename prefix matching `r<stage:01x><room:02x>_`.
   - The cuts for that room are queued to the background worker thread.
   - When Jill turns a corner and the camera switches cuts within that room, the texture data is already decoded in memory (`Ready` state) or loaded into an OpenGL texture.
   - Cache capacity (default 256 MiB / 32 entries) uses LRU eviction based on access tick counters.

4. **HD Composition into `s_hr_fbo`**:
   - When the frame begins and the display buffer is cleared (via `gp0_exec_fill_rect` / `gpu_fill`), the engine blits the current HD background texture into the active backbuffer rectangle of `s_hr_fbo`.
   - The blit uses a fullscreen/subrect quad with normalized UVs `(0,0)..(1,1)`.
   - 3D actor meshes (Jill, Nemesis, enemies) are then drawn on top with depth testing and depth writing enabled.
   - Foreground occlusion quads (`POLY_FT4`) sample from the HD texture using screen-space UVs, ensuring that pillars and obstacles cleanly occlude 3D actors without pixelation seams.

---

## 3. Caveats

1. **FMV & 24-Bit Modes**:
   - During FMV playback or MDEC video sequences, `gpu_display_is_depth24()` is true, and the camera state variables in RAM may hold stale values from the prior gameplay room.
   - HD background rendering must be gated with `if (!gpu_display_is_depth24() && re3_hd_bg_has_hd_current())`.
2. **Partial Packs & Missing Cuts**:
   - Community texture packs may omit certain camera angles.
   - When `re3_hd_bg_has_hd_current()` returns false, the renderer must seamlessly fall back to native PS1 VRAM rendering without crashing or logging error spam.
3. **Thread Safety & OpenGL Context**:
   - OpenGL textures must be created (`glGenTextures`) and uploaded (`glTexImage2D`) on the thread owning the active OpenGL context (main render thread).
   - The asynchronous worker thread must only read disk files and decode PNG bytes into host RGBA8 memory buffers (`DecodedImage`). The upload to GL texture occurs on the render thread upon first use.
4. **Path Traversal Security**:
   - Pack names from `settings.toml` must be validated against `..` components and absolute drive roots to prevent directory traversal outside `mods/textures/`.

---

## 4. Conclusion & Interface Contracts

### 4.1 Interface Contract: `psxrecomp/runtime/include/re3_hd_backgrounds.h`

```c
#ifndef RE3_HD_BACKGROUNDS_H
#define RE3_HD_BACKGROUNDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Diagnostic metrics for HD Background engine.
 */
typedef struct {
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    uint64_t prefetches;
    size_t memory_used_bytes;
    size_t memory_budget_bytes;
    uint32_t active_entries;
} re3_hd_bg_metrics_t;

/**
 * Initializes the HD pre-rendered backgrounds engine.
 * @param pack_path Absolute or relative path to the active texture pack directory
 *                  (e.g., "mods/textures/SeamlessHD").
 */
void re3_hd_bg_init(const char* pack_path);

/**
 * Shuts down the background engine, stops worker threads, and releases all GL textures.
 */
void re3_hd_bg_shutdown(void);

/**
 * Notifies the engine of an updated game camera state.
 * Triggers eager prefetching of all room cuts if the room has changed.
 * @param stage Stage ID (0x000D1F08)
 * @param room  Room ID (0x000D1F09)
 * @param cut   Cut/Camera angle ID (0x000D1F0A)
 */
void re3_hd_bg_on_room_change(uint8_t stage, uint8_t room, uint8_t cut);

/**
 * Returns true if an HD background is currently available and ready for the active cut.
 */
bool re3_hd_bg_has_hd_current(void);

/**
 * Returns the OpenGL texture handle (GLuint) for the currently active background, or 0.
 */
uint32_t re3_hd_bg_get_current_gl_tex(void);

/**
 * Blits the current HD background texture into the target FBO (s_hr_fbo).
 * @param fbo    Target OpenGL Framebuffer Object handle
 * @param width  Target framebuffer width in pixels
 * @param height Target framebuffer height in pixels
 * @return true if successfully rendered, false if fallback to VRAM is required.
 */
bool re3_hd_bg_render_current(uint32_t fbo, int width, int height);

/**
 * Resolves a background file path on disk for given (stage, room, cut) using
 * deterministic RESHDP candidate matching and fallback rules.
 * @param stage    Stage ID
 * @param room     Room ID
 * @param cut      Cut ID
 * @param out_path Buffer to receive resolved absolute/normalized path
 * @param max_len  Capacity of out_path buffer
 * @return Pointer to out_path on success, NULL if no file exists.
 */
const char* re3_hd_bg_resolve_file(uint8_t stage, uint8_t room, uint8_t cut,
                                   char* out_path, size_t max_len);

/**
 * Calculates screen-space UV coordinates for an occlusion mask quad.
 * @param x      Quad X coordinate on 320x240 screen
 * @param y      Quad Y coordinate on 320x240 screen
 * @param w      Quad width on screen
 * @param h      Quad height on screen
 * @param out_uv Output array of 4 floats: [u_min, v_min, u_max, v_max]
 */
void re3_hd_bg_get_screen_uv(int x, int y, int w, int h, float out_uv[4]);

/**
 * Retrieves engine metrics and statistics.
 */
void re3_hd_bg_get_metrics(re3_hd_bg_metrics_t* out_metrics);

#ifdef __cplusplus
}
#endif

#endif /* RE3_HD_BACKGROUNDS_H */
```

### 4.2 Data Structures for `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`

```cpp
#include "re3_hd_backgrounds.h"
#include "re3_textures.h"
#include "../third_party/stb_image.h"

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <filesystem>
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace fs = std::filesystem;

// Key identifying a specific background: (stage << 16) | (room << 8) | cut
inline uint32_t make_bg_key(uint8_t stage, uint8_t room, uint8_t cut) {
    return (static_cast<uint32_t>(stage) << 16) |
           (static_cast<uint32_t>(room) << 8) |
           static_cast<uint32_t>(cut);
}

enum class DecodeState {
    None,
    Queued,
    Loading,
    Ready,      // Pixels decoded, ready for GL upload
    Uploaded,   // Uploaded to GL texture
    Failed
};

struct DecodedImage {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba;
};

struct BgCacheEntry {
    uint32_t key = 0;
    uint8_t stage = 0;
    uint8_t room = 0;
    uint8_t cut = 0;
    std::string file_path;
    DecodeState state = DecodeState::None;
    std::shared_ptr<DecodedImage> image;
    uint32_t gl_tex_id = 0;
    size_t bytes = 0;
    uint64_t last_used = 0;
};
```

### 4.3 Deterministic File Resolver Implementation Logic

```cpp
const char* re3_hd_bg_resolve_file(uint8_t stage, uint8_t room, uint8_t cut,
                                   char* out_path, size_t max_len) {
    if (!out_path || max_len == 0) return nullptr;
    out_path[0] = '\0';

    if (!re3_textures_is_pack_valid()) return nullptr;

    fs::path bg_dir = fs::path(re3_textures_get_pack_path()) / "backgrounds";
    std::error_code ec;
    if (!fs::exists(bg_dir, ec) || !fs::is_directory(bg_dir, ec)) return nullptr;

    // Ordered candidate list
    char cand[64];
    std::vector<std::string> candidates;

    // 1. Standard RESHDP: R<stage><room:02X>_<cut:02X>.png
    std::snprintf(cand, sizeof(cand), "R%1X%02X_%02X.png", stage, room, cut);
    candidates.push_back(cand);

    // 2. Single-digit cut: R<stage><room:02X>_<cut>.png
    std::snprintf(cand, sizeof(cand), "R%1X%02X_%u.png", stage, room, cut);
    if (std::find(candidates.begin(), candidates.end(), cand) == candidates.end())
        candidates.push_back(cand);

    // 3. Lowercase RESHDP: r<stage><room:02x>_<cut:02x>.png
    std::snprintf(cand, sizeof(cand), "r%1x%02x_%02x.png", stage, room, cut);
    if (std::find(candidates.begin(), candidates.end(), cand) == candidates.end())
        candidates.push_back(cand);

    // 4. Lowercase single-digit cut: r<stage><room:02x>_<cut>.png
    std::snprintf(cand, sizeof(cand), "r%1x%02x_%u.png", stage, room, cut);
    if (std::find(candidates.begin(), candidates.end(), cand) == candidates.end())
        candidates.push_back(cand);

    // 5. Uppercase .PNG variations
    size_t count = candidates.size();
    for (size_t i = 0; i < count; ++i) {
        std::string upper_ext = candidates[i];
        if (upper_ext.size() >= 4 && upper_ext.substr(upper_ext.size() - 4) == ".png") {
            upper_ext.replace(upper_ext.size() - 4, 4, ".PNG");
            candidates.push_back(upper_ext);
        }
    }

    // Direct filesystem probe
    for (const auto& name : candidates) {
        fs::path p = bg_dir / name;
        if (fs::exists(p, ec) && fs::is_regular_file(p, ec)) {
            std::string p_str = p.generic_string();
            std::strncpy(out_path, p_str.c_str(), max_len - 1);
            out_path[max_len - 1] = '\0';
            return out_path;
        }
    }

    return nullptr;
}
```

### 4.4 Call Graph & Integration Points

```
[ RE3Recomp Game Execution ]
       │
       ├─► main() / init:
       │     └─► re3_textures_init() ────────► re3_hd_bg_init(pack_path)
       │
       ├─► gl_renderer_present_vram():
       │     │
       │     ├─► psx_read_byte(0x000D1F08)  [Stage: 1..5]
       │     ├─► psx_read_byte(0x000D1F09)  [Room: 0x00..0x25]
       │     └─► psx_read_byte(0x000D1F0A)  [Cut: 0x00..0xFF]
       │           │
       │           ▼
       │     re3_hd_bg_on_room_change(stage, room, cut)
       │           │
       │           ├─► (If room changed) Enumerate cuts -> Push to Async Worker Queue
       │           │                                             │
       │           │                                             ▼
       │           │                             [ Background Worker Thread ]
       │           │                             stbi_load_from_memory() -> RGBA8
       │           │
       │           └─► (If current cut ready) Upload to GL Texture on render thread
       │
       ├─► Frame Rendering (gpu_fill / display clear):
       │     │
       │     ▼
       │   re3_hd_bg_render_current(s_hr_fbo, width, height)
       │     │
       │     ├─► Bind s_hr_fbo
       │     ├─► Bind HD GL texture
       │     ├─► Draw screen quad to active backbuffer rect (0, 0, 320*S, 240*S)
       │     └─► Unbind
       │
       ├─► 3D Actor Meshes (Jill, Nemesis, Enemies):
       │     └─► Draw directly over HD background in s_hr_fbo with depth test/write
       │
       ├─► Foreground Occlusion Quads (POLY_FT4):
       │     └─► re3_hd_bg_get_screen_uv(x, y, w, h, out_uv) -> Sample HD texture
       │
       └─► Shutdown / Teardown:
             └─► re3_textures_shutdown() ────► re3_hd_bg_shutdown()
```

---

## 5. Verification Method

### 5.1 Test Harness Verification
1. Run Tier 1 Feature tests (Features 2, 3, 4, 5, 6):
   ```powershell
   python -m unittest tests/e2e/test_tier1_features.py -v
   ```
   Must pass:
   - `TestFeature02StateTracking`: `test_01` to `test_05`
   - `TestFeature03ReshdpResolution`: `test_01` to `test_05`
   - `TestFeature04AsyncPrefetchCache`: `test_01` to `test_05`
   - `TestFeature05FboComposition`: `test_01` to `test_05`
   - `TestFeature06OcclusionMaskAlignment`: `test_01` to `test_05`

2. Run Tier 2 Boundaries tests:
   ```powershell
   python -m unittest tests/e2e/test_tier2_boundaries.py -v
   ```
   Must pass `test_b04` through `test_b15`.

3. Run Tier 5 Adversarial tests:
   ```powershell
   python -m unittest tests/e2e/test_tier5_adversarial.py -v
   ```
   Must pass `test_a01`, `test_a02`, `test_a03`, `test_a09`, `test_a10`, `test_a11`, `test_a12`.

### 5.2 Build Verification
- When implemented in Milestone 2, build RE3Recomp runtime via:
  ```powershell
  powershell -ExecutionPolicy Bypass -File "build.ps1"
  ```
  Must compile without errors or warnings.

### 5.3 Invalidation Conditions
- The findings would be invalidated if:
  1. RE3 stored room/cut numbers at dynamic heap pointers instead of fixed static RAM addresses `0x000D1F08..0x000D1F0A` (refuted: verified in `gpu_gl_renderer.c` and decompilation memory maps).
  2. The game bypassed `s_hr_fbo` during 15-bit gameplay presentation (refuted: verified in `gpu_gl_renderer.c` line 8283).
  3. `stb_image` were unavailable (refuted: verified in `psx_window_icon.cpp` and `hd_texture_pack.cpp`).
