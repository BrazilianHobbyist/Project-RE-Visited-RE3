# Handoff Report: Explorer M4-1 (UI Screen Detection & Backdrop Resolution)

**Agent**: Explorer M4-1  
**Target Milestone**: Milestone 4 (HD User Interface & Menus - Feature 12 & Feature 13)  
**Date**: 2026-10-18T07:16:00Z  

---

## 1. Observation

### 1.1. Feature 12 Definition in E2E Test Suite
The E2E test suite in `tests/e2e/test_tier1_features.py` defines and verifies Feature 12 at lines 508–542:
```python
# ============================================================================
# Feature 12: HD UI 2D Screen Backdrops (M4)
# ============================================================================
class TestFeature12HdUiBackdrops(BaseTier1Test):
    """Feature 12: 1080p/4K background replacement for inventory, status, map, epilogues, clock."""

    def setUp(self):
        self.ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")

    def test_01_inventory_backdrop_resolution(self):
        info = self.ui.load_backdrop("inventory")
        self.assertTrue(info["has_hd"])
        self.assertFalse(info["fallback_native_ui"])

    def test_02_ecg_status_backdrop_states(self):
        for cond in [CONDITION_FINE, CONDITION_CAUTION, CONDITION_DANGER, CONDITION_POISON]:
            p = self.ui.resolve_status_backdrop(cond)
            self.assertIsNotNone(p)
            self.assertTrue(os.path.isfile(p))

    def test_03_map_screen_backdrop(self):
        info = self.ui.load_backdrop("map")
        self.assertTrue(info["has_hd"])

    def test_04_epilogue_gallery_backdrops(self):
        for i in range(8):
            info = self.ui.load_backdrop(f"epilogue_{i}")
            self.assertTrue(info["has_hd"])

    def test_05_clock_and_save_backdrops(self):
        info_clock = self.ui.load_backdrop("clock")
        info_save = self.ui.load_backdrop("save")
        self.assertTrue(info_clock["has_hd"])
        self.assertTrue(info_save["has_hd"])
```

### 1.2. Boundary, Interaction, and Adversarial E2E Tests
1. **Tier 2 Boundaries (`tests/e2e/test_tier2_boundaries.py:237-252`)**:
   - `test_b30_ui_health_condition_unknown`: condition `0xFF` must fall back gracefully to `status_fine.png`.
   - `test_b31_ui_epilogue_boundary_indices`: indices 0..7 return `has_hd = True`; index 8 is out of bounds and returns `has_hd = False`.
2. **Tier 3 Interactions (`tests/e2e/test_tier3_interactions.py:122-190, 256-270`)**:
   - `test_i05_3d_item_inspection_in_hd_inventory`: inventory HD backdrop is active while 3D weapon replacement renders simultaneously on top.
   - `test_i06_combat_damage_ecg_status_transition`: condition changes FINE -> DANGER -> POISON update ECG backdrop dynamically.
   - `test_i08_opening_map_screen_from_active_room`: map backdrop loads cleanly while room camera is active.
   - `test_i09_typewriter_save_screen_transition`: typewriter save dialog backdrop loads.
   - `test_i10_epilogue_gallery_navigation`: epilogues 0 to 7 navigate cleanly.
   - `test_i15_partial_pack_cross_subsystem_fallback`: `IncompletePack` without UI backdrops returns `fallback_native_ui = True` without crashing.
3. **Tier 4 Scenarios (`tests/e2e/test_tier4_scenarios.py:175-210`)**:
   - `test_s03_inventory_and_status_menu_session`: validates composite session involving inventory, herb icon replacement, 3D model inspect, ECG transitions, map, and resume.
4. **Tier 5 Adversarial (`tests/e2e/test_tier5_adversarial.py:101-145`)**:
   - Corrupted PNGs, bad CRC, path traversal (`../..` escapes) return graceful fallback.

### 1.3. UI Directory Structure & Filenames in Test Fixtures
In `tests/e2e/harness/fixtures.py:116-132`:
```python
        # UI Backdrops
        ui_screens = [
            "inventory.png", "status_fine.png", "status_caution.png",
            "status_danger.png", "status_poison.png", "map.png",
            "clock.png", "save.png"
        ]
        for name in ui_screens:
            with open(os.path.join(ui_dir, name), "wb") as f:
                f.write(create_solid_rgba_png(640, 480, 20, 40, 60))

        for i in range(8):
            with open(os.path.join(ui_dir, f"epilogue_{i}.png"), "wb") as f:
                f.write(create_solid_rgba_png(320, 240, 70 + i * 10, 50, 90))

        # UI Sprite
        with open(os.path.join(ui_dir, f"{UI_HERB_HASH:016x}.png"), "wb") as f:
            f.write(create_solid_rgba_png(64, 64, 0, 255, 0))
```

In `tests/e2e/harness/ui_pipeline.py:13-26`:
```python
UI_BACKDROP_SCREENS = {
    "inventory": "inventory.png",
    "status_fine": "status_fine.png",
    "status_caution": "status_caution.png",
    "status_danger": "status_danger.png",
    "status_poison": "status_poison.png",
    "map": "map.png",
    "clock": "clock.png",
    "save": "save.png",
}
for i in range(8):
    UI_BACKDROP_SCREENS[f"epilogue_{i}"] = f"epilogue_{i}.png"
```

### 1.4. Memory State Addresses in Codebase
In `tests/e2e/harness/memory_state.py:15-26` and `psxrecomp/runtime/src/main.cpp:5560-5576`:
- `0x000CCC97`: Player Condition byte:
  - `0x04`: `CONDITION_FINE`
  - `0x03`: `CONDITION_CAUTION`
  - `0x02`: `CONDITION_DANGER`
  - `0x01`: `CONDITION_POISON`
- `0x000D1F3E`: Epilogue Count (0..8) in secret gallery.
- `0x000D225E`: Inventory Max Slots (8 or 10).
- `0x000D2134`: Inventory Item Slots Base.
- `0x000D1F08`: Stage ID.
- `0x000D1F09`: Room ID.
- `0x000D1F0A`: Cut ID.

### 1.5. Existing GPU Hook Patterns in `gpu.c` & `gpu_gl_renderer.c`
1. Room backgrounds blit in `psxrecomp/runtime/src/gpu_gl_renderer.c:3988-3993`:
   ```c
   if (!gpu_display_is_depth24() && (w == 320 && h == 240) && re3_hd_bg_has_hd_current()) {
       glViewport(x * s_hr_scale, y * s_hr_scale, w * s_hr_scale, h * s_hr_scale);
       glDisable(GL_SCISSOR_TEST);
       re3_hd_bg_render_current(s_hr_fbo, w * s_hr_scale, h * s_hr_scale);
       glViewport(0, 0, VRAM_W * s_hr_scale, VRAM_H * s_hr_scale);
   }
   ```
2. Screen-spanning 2D quad geometry detection in `psxrecomp/runtime/src/gpu.c:3093-3115`:
   ```c
   /* Must span the full display width and a real vertical extent. */
   if (minx > X + EDGE || maxx < X + W - EDGE || (maxy - miny) < 64) return 0;
   /* Axis-aligned: every vertex X sits at either the min or the max edge, and
    * every Y at the top or bottom edge (a true screen-space rectangle). */
   for (int i = 0; i < 4; i++) {
       int xe = (vx[i] - minx <= EDGE) || (maxx - vx[i] <= EDGE);
       int ye = (vy[i] - miny <= EDGE) || (maxy - vy[i] <= EDGE);
       if (!xe || !ye) return 0;
   }
   ```
3. Fullscreen quad detection in `psxrecomp/runtime/src/gpu.c:4836-4840`:
   ```c
   if (vx[0] == vx[2] && vx[1] == vx[3] &&
       vy[0] == vy[1] && vy[2] == vy[3] &&
       vx[1] > vx[0] && vy[2] > vy[0] &&
       vx[0] <= 0 && vx[1] >= ws_disp_w() &&
       vy[0] <= 0 && vy[2] >= ws_disp_h())
   ```
4. Texture unit binding in `psxrecomp/runtime/src/gpu_gl_renderer.c:3777-3784`:
   `s_tb_hd_tex` and `s_tb_is_hd` control shader branching in `TEX_FS`.
5. Configuration flag in `psxrecomp/runtime/include/re3_textures.h:16`:
   `g_re3_tex_config.hd_ui` (boolean flag).

---

## 2. Logic Chain

1. **E2E Requirement Alignment**:
   From Observation 1.1 and 1.2, the E2E suite requires:
   - `load_backdrop(screen_name)` returning metadata dictionary with `has_hd`, `file_path`, `width`, `height`, `gl_tex_id`, `fallback_native_ui`.
   - `resolve_status_backdrop(condition)` returning path to condition-specific status image, with unknown values defaulting to `status_fine.png`.
   - 8 epilogues (`epilogue_0` to `epilogue_7`), with index 8 rejected.
   - Interoperability with 3D models (Observation 1.2, `test_i05`), where 3D item examination models can be rendered in the same frame over the HD inventory backdrop.

2. **Candidate File Path Strategy**:
   From Observation 1.3, assets are stored in `<pack_path>/ui/`.
   To support real-world mod packs (including Seamless HD Project, Classic HD, and user variations), the resolution engine must test candidate filenames in prioritized order:
   - Screen `inventory` -> `inventory.png`, `inventory.PNG`, `INVENTORY.PNG`, `Inventory.png`
   - Screen `status_fine` -> `status_fine.png`, `status_fine.PNG`, `STATUS_FINE.PNG`, fallback to `status.png`
   - Screen `status_caution` -> `status_caution.png`, `status_caution.PNG`, `STATUS_CAUTION.PNG`, fallback to `status.png`
   - Screen `status_danger` -> `status_danger.png`, `status_danger.PNG`, `STATUS_DANGER.PNG`, fallback to `status.png`
   - Screen `status_poison` -> `status_poison.png`, `status_poison.PNG`, `STATUS_POISON.PNG`, fallback to `status.png`
   - Screen `map` -> `map.png`, `map.PNG`, `MAP.PNG`, `Map.png`
   - Screen `clock` -> `clock.png`, `clock.PNG`, `CLOCK.PNG`, `Clock.png`
   - Screen `save` -> `save.png`, `save.PNG`, `SAVE.PNG`, `Save.png`, fallback `typewriter.png`
   - Screen `epilogue_0`..`epilogue_7` -> `epilogue_<i>.png`, `epilogue_<i>.PNG`, `EPILOGUE_<i>.PNG`, `epilogue<i>.png`
   - Fallback case-insensitive scan of the `ui/` directory if exact matches fail, mirroring the battle-tested resolver in `re3_hd_backgrounds.cpp:465-480`.
   - Path traversal guard (Observation 1.2, `test_a10`) using `std::filesystem::weakly_canonical` to prevent `..` escapes.

3. **UI Screen Detection & Quad Interception Mechanism**:
   From Observation 1.4 and 1.5:
   - In PS1 RE3, when a UI screen opens:
     a) **Memory State Signals**:
        - Health condition at `0x000CCC97` reliably determines ECG state.
        - Epilogue count at `0x000D1F3E` determines unlocked cards.
        - Game mode flags identify whether the player is in Inventory, Map, or Gameplay.
     b) **Geometry Interception**:
        - In `gpu.c` (`gp0_exec_textured_quad` / `gp0_exec_textured_rect`), screen-space 2D backdrop quads have bounding dimensions spanning the display (width >= 320, height >= 240, or paired 160/256-wide tiles).
        - When a menu quad is identified and `g_re3_tex_config.hd_ui` is true, the low-res PS1 quad draw is intercepted:
          1. The corresponding HD backdrop (1080p/4K) is blitted into `s_hr_fbo` using a full-screen shader pass (similar to `re3_hd_bg_render_current`).
          2. The native low-res textured quad is suppressed (or zero-alpha / bypassed).
          3. Dynamic overlay preservation: All subsequent text fonts, cursors, item icons, and 3D item inspection meshes are drawn onto `s_hr_fbo` normally, guaranteeing pixel-perfect interactive UI over the HD backdrop.

4. **Interface Contract Design (`re3_hd_ui.h`)**:
   From Observation 1.1, 1.4, and the modular architecture established in `re3_hd_backgrounds.h` and `re3_hd_textures.h`:
   - Pure C-linkage header (`extern "C"`).
   - Clean lifecycle (`init`, `shutdown`, `is_active`).
   - Unified backdrop resolution, loading, and FBO blit APIs.
   - Memory-assisted detection helper (`re3_hd_ui_resolve_status_backdrop` from `0x000CCC97`).
   - Fullscreen quad interception test function (`re3_hd_ui_intercept_menu_quad`).
   - UI sprite replacement hash lookup for Feature 13 (`re3_hd_ui_bind_sprite`).
   - Performance metrics struct (`re3_hd_ui_metrics_t`).

---

## 3. Caveats

1. **PS1 Menu Tiling Variations**: On real hardware, Capcom's engine occasionally draws backdrops using two 160x240 quads or one 256x240 quad and one 64x240 quad due to PS1 256x256 VRAM page limits. The quad interception logic must latch on the first tile and suppress the second tile to prevent double-blitting the HD backdrop.
2. **Read-Only Scope**: In accordance with the Explorer archetype, no source files outside `.agents/teamwork/explorer_m4_1/` were modified. Implementation of `re3_hd_ui.h`, `re3_hd_ui.cpp`, and integration hooks in `gpu.c` / `gpu_gl_renderer.c` remains for the M4 implementer.

---

## 4. Conclusion & Proposed Header Design

### 4.1. Screen Names & Candidate Path Table

| Screen ID | Canonical Filename | Case Variations & Fallbacks | Trigger Condition / Address |
| :--- | :--- | :--- | :--- |
| `inventory` | `inventory.png` | `inventory.PNG`, `INVENTORY.PNG`, `Inventory.png` | Inventory menu opened (Start / Triangle) |
| `status_fine` | `status_fine.png` | `status_fine.PNG`, `STATUS_FINE.PNG`, `status.png` | `0x000CCC97 == 0x04` or unknown condition |
| `status_caution` | `status_caution.png` | `status_caution.PNG`, `STATUS_CAUTION.PNG`, `status.png` | `0x000CCC97 == 0x03` |
| `status_danger` | `status_danger.png` | `status_danger.PNG`, `STATUS_DANGER.PNG`, `status.png` | `0x000CCC97 == 0x02` |
| `status_poison` | `status_poison.png` | `status_poison.PNG`, `STATUS_POISON.PNG`, `status.png` | `0x000CCC97 == 0x01` |
| `map` | `map.png` | `map.PNG`, `MAP.PNG`, `Map.png` | Map screen opened (Select) |
| `clock` | `clock.png` | `clock.PNG`, `CLOCK.PNG`, `Clock.png` | Clock puzzle / time screen |
| `save` | `save.png` | `save.PNG`, `SAVE.PNG`, `Save.png`, `typewriter.png` | Typewriter save prompt |
| `epilogue_0`..`7` | `epilogue_<i>.png` | `epilogue_<i>.PNG`, `EPILOGUE_<i>.PNG`, `epilogue<i>.png` | Epilogue gallery view (`0x000D1F3E`) |

### 4.2. Proposed Interface Contract: `psxrecomp/runtime/include/re3_hd_ui.h`

```c
#ifndef RE3_HD_UI_H
#define RE3_HD_UI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RE3_UI_SCREEN_NONE = 0,
    RE3_UI_SCREEN_INVENTORY,
    RE3_UI_SCREEN_STATUS_FINE,
    RE3_UI_SCREEN_STATUS_CAUTION,
    RE3_UI_SCREEN_STATUS_DANGER,
    RE3_UI_SCREEN_STATUS_POISON,
    RE3_UI_SCREEN_MAP,
    RE3_UI_SCREEN_CLOCK,
    RE3_UI_SCREEN_SAVE,
    RE3_UI_SCREEN_EPILOGUE_0,
    RE3_UI_SCREEN_EPILOGUE_1,
    RE3_UI_SCREEN_EPILOGUE_2,
    RE3_UI_SCREEN_EPILOGUE_3,
    RE3_UI_SCREEN_EPILOGUE_4,
    RE3_UI_SCREEN_EPILOGUE_5,
    RE3_UI_SCREEN_EPILOGUE_6,
    RE3_UI_SCREEN_EPILOGUE_7,
    RE3_UI_SCREEN_COUNT
} re3_ui_screen_t;

typedef struct {
    bool has_hd;
    bool fallback_native_ui;
    uint32_t gl_tex_id;
    int width;
    int height;
    char file_path[512];
    char screen_name[64];
} re3_ui_backdrop_info_t;

typedef struct {
    uint64_t backdrop_hits;
    uint64_t backdrop_misses;
    uint64_t sprite_hits;
    uint64_t sprite_misses;
    uint32_t active_textures;
} re3_hd_ui_metrics_t;

/**
 * Initializes the HD UI subsystem with the active texture pack path.
 * @param pack_path Path to the texture pack directory (e.g. "mods/textures/SeamlessHD").
 */
void re3_hd_ui_init(const char* pack_path);

/**
 * Shuts down the HD UI subsystem and frees all allocated GL textures and caches.
 */
void re3_hd_ui_shutdown(void);

/**
 * Returns true if the HD UI subsystem is initialized and active.
 */
bool re3_hd_ui_is_active(void);

/**
 * Resolves the candidate file path for a UI screen backdrop.
 * Performs direct lookup, uppercase/extension variants, and case-insensitive fallback.
 *
 * @param screen_name Canonical name (e.g. "inventory", "map", "status_fine", "epilogue_0")
 * @param out_path    Buffer to receive the resolved filesystem path
 * @param max_len     Capacity of out_path buffer
 * @return Pointer to out_path on success, NULL if not found.
 */
const char* re3_hd_ui_resolve_backdrop_path(const char* screen_name, char* out_path, size_t max_len);

/**
 * Resolves the status ECG backdrop based on player condition byte (0x000CCC97).
 * Condition mappings:
 *   0x04 -> status_fine
 *   0x03 -> status_caution
 *   0x02 -> status_danger
 *   0x01 -> status_poison
 *   Unknown (e.g. 0xFF) -> status_fine
 */
const char* re3_hd_ui_resolve_status_backdrop(uint8_t condition, char* out_path, size_t max_len);

/**
 * Loads and caches a UI backdrop texture into OpenGL.
 *
 * @param screen_name Canonical screen name
 * @param out_info    Pointer to receive loaded backdrop metadata
 * @return true if HD backdrop is loaded, false if native fallback applies.
 */
bool re3_hd_ui_load_backdrop(const char* screen_name, re3_ui_backdrop_info_t* out_info);

/**
 * Returns true if an HD backdrop texture is available and cached for the screen.
 */
bool re3_hd_ui_has_backdrop(const char* screen_name);

/**
 * Returns the GLuint texture handle for the specified backdrop, or 0 if missing.
 */
uint32_t re3_hd_ui_get_backdrop_gl_tex(const char* screen_name);

/**
 * Blits an HD backdrop directly into the target FBO (s_hr_fbo) at native aspect ratio.
 *
 * @param screen_name Canonical screen name
 * @param fbo         Target Framebuffer Object handle
 * @param width       Target FBO viewport width
 * @param height      Target FBO viewport height
 * @return true on successful blit, false if native UI fallback is needed.
 */
bool re3_hd_ui_render_backdrop(const char* screen_name, uint32_t fbo, int width, int height);

/**
 * Tests whether a drawn quad represents a full-screen or screen-spanning 2D menu quad.
 */
bool re3_hd_ui_is_fullscreen_quad(int32_t x0, int32_t y0, int32_t w, int32_t h);

/**
 * Detects the active UI screen from guest RAM memory indicators (0x000CCC97, 0x000D1F3E, etc.).
 * Returns canonical screen name string, or NULL if standard room gameplay is active.
 */
const char* re3_hd_ui_detect_screen_from_memory(void);

/**
 * Intercepts a potential menu quad in the GPU pipeline.
 * If recognized as a UI backdrop, renders HD backdrop to s_hr_fbo and returns true
 * (signaling that the native low-res quad should be skipped).
 */
bool re3_hd_ui_intercept_menu_quad(int32_t x0, int32_t y0, int32_t w, int32_t h,
                                  uint16_t tpage, uint16_t clut,
                                  uint32_t fbo, int fb_w, int fb_h);

/**
 * Binds and returns GLuint texture handle for a replacement UI sprite (Feature 13).
 * Returns 0 if no replacement exists (transparent fallback).
 */
uint32_t re3_hd_ui_bind_sprite(uint64_t hash);

/**
 * Retrieves performance metrics for the HD UI subsystem.
 */
void re3_hd_ui_get_metrics(re3_hd_ui_metrics_t* out_metrics);

#ifdef __cplusplus
}
#endif

#endif /* RE3_HD_UI_H */
```

---

## 5. Verification Method

To verify these findings independently:

1. **Verify E2E Tests for Feature 12 & Feature 13**:
   Run the Tier 1 E2E test suite using Python:
   ```bash
   python tests/e2e/run_e2e_tests.py
   ```
   Or specifically:
   ```bash
   python -m unittest tests.e2e.test_tier1_features.TestFeature12HdUiBackdrops
   python -m unittest tests.e2e.test_tier1_features.TestFeature13HdUiSprites
   ```
   *Expected Result*: All 5 test cases of Feature 12 and 5 test cases of Feature 13 pass.

2. **Verify Boundary and Interaction Tests**:
   ```bash
   python -m unittest tests.e2e.test_tier2_boundaries.TestTier2Boundaries.test_b30_ui_health_condition_unknown
   python -m unittest tests.e2e.test_tier2_boundaries.TestTier2Boundaries.test_b31_ui_epilogue_boundary_indices
   python -m unittest tests.e2e.test_tier3_interactions.TestTier3Interactions.test_i05_3d_item_inspection_in_hd_inventory
   python -m unittest tests.e2e.test_tier3_interactions.TestTier3Interactions.test_i06_combat_damage_ecg_status_transition
   ```
   *Expected Result*: All tests pass without assertion failures.

3. **Verify File and Memory Layout Inspection**:
   - Inspect `tests/e2e/harness/memory_state.py` lines 15–26 for `0x000CCC97` (`ADDR_PLAYER_CONDITION`) and `0x000D1F3E` (`ADDR_EPILOGUE_COUNT`).
   - Inspect `psxrecomp/runtime/src/gpu_gl_renderer.c` lines 3988–3993 for FBO blit pattern.
   - Inspect `psxrecomp/runtime/src/gpu.c` lines 3093–3124 and 4836–4840 for screen-spanning quad detection pattern.
