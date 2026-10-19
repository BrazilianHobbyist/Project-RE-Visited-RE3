# Project: RE3Recomp Native HD Texture & 3D Model Pack Engine

## Architecture

RE3Recomp is a static binary recompilation of Resident Evil 3: Nemesis (SLUS-00923) targeting PC/Windows via OpenGL and SDL2. The HD Texture and 3D Model Engine is architected as a modular, high-performance subsystem:

```
[ settings.toml ] ──────────────► [ Launcher: ShowTexturesDialog ]
       │                                     │
       ▼                                     ▼
[ re3_texture_config ] ──────────► [ mods/textures/<pack>/ ]
       │                                     │
       ├─────────────────────────────────────┼──────────────────────────────┐
       ▼                                     ▼                              ▼
[ re3_hd_backgrounds ]             [ re3_hd_textures ]               [ re3_hd_ui ]
- Eager async decode (stb)         - 64-bit xxHash64 fingerprint     - 1080p/4K backdrops
- Stage/Room/Cut (0xD1F08/9/A)     - Aniso 16x + trilinear mipmap    - Screen-space quads
- R<stage><room>_<cut>.png         - Normalized UV mapping           - UI sprite replacements
- Direct blit to s_hr_fbo          - Transparent PS1 VRAM fallback   - Dynamic overlay preserv.
- Mask quad screen-space align     - dump_textures PNG exporter                     │
       │                                     │                                      │
       └─────────────────────────────────────┼──────────────────────────────────────┘
                                             ▼
                                  [ gpu_gl_renderer.c ]
                                  - s_hr_fbo up to 4K
                                  - TEX_FS shader branching (u_is_hd)
                                  - Actor polygon tagging
```

### Key Subsystems:
1. **Configuration & Common State (`re3_textures.h`)**:
   Holds `re3_texture_config_t` with pack name, enabled flags (`hd_backgrounds`, `hd_models`, `hd_ui`, `dump_textures`), active paths, and state accessors.
2. **HD Pre-rendered Backgrounds Engine (`re3_hd_backgrounds.h / .cpp`)**:
   Tracks memory states `0x000D1F08` (Stage), `0x000D1F09` (Room), `0x000D1F0A` (Cut) via `psx_read_byte()`. Pre-fetches room cuts asynchronously to avoid stutter. Blits 1440p/4K backgrounds into `s_hr_fbo` at the start of each frame and aligns foreground mask quads with 1:1 screen-space UVs.
3. **3D Model & Actor HD Texture Subsystem (`re3_hd_textures.h / .cpp`)**:
   Computes 64-bit `xxHash64` over composite texture payloads `(tpage, clut, limits, palette, texels)`. Binds replacements from `mods/textures/<pack>/replacements/<hash>.png` on texture unit 2. Normalizes UVs, generates trilinear mipmaps and sets 16x anisotropic filtering. Transparently falls back to `s_raw_tex` (`u_is_hd = 0`) when missing.
4. **HD UI & 2D Menus Subsystem (`re3_hd_ui.h / .cpp`)**:
   Intercepts full-screen menu quads and draws 1080p/4K backdrops for inventory, status, map, epilogues, and clock from `mods/textures/<pack>/ui/<screen>.png` into `s_hr_fbo`, preserving interactive text/sprites/cursors on top.
5. **Texture Dumping Tooling**:
   Activated via `dump_textures = true`. Saves decoded PS1 textures with xxHash64 fingerprints and IDs as PNG files in `dump/textures/` with in-memory session deduplication.
6. **Win32 Launcher & GUI (`launcher/main.cpp`)**:
   Adds "Mods & Texturas HD" modal dialog (`ShowTexturesDialog`), enumerates `mods/textures/`, toggles feature checkboxes, persists to `settings.toml`, and opens the mods directory via `ShellExecuteW`.

---

## Feature Inventory

| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | Configuration & Settings Parser | Parse `[textures]` section (`pack`, `hd_backgrounds`, `hd_models`, `hd_ui`, `dump_textures`) in `settings.toml` and expose global config | M1 | ORIGINAL_REQUEST §R5 |
| 2 | Stage/Room/Cut State Tracking | Memory reader for 0x000D1F08, 0x000D1F09, 0x000D1F0A to identify active camera and cut | M2 | ORIGINAL_REQUEST §R1 |
| 3 | RESHDP Background Loading & File Resolution | Resolver for `R<stage><room:02X>_<cut:02X>.png` with fallback variations in `mods/textures/<pack>/backgrounds/` | M2 | ORIGINAL_REQUEST §R1 |
| 4 | Asynchronous Background Pre-fetcher & LRU Cache | Multi-threaded loader that pre-loads room angles into memory/GL textures without hitching | M2 | ORIGINAL_REQUEST §R1 |
| 5 | Background Rendering to `s_hr_fbo` | High-resolution background composition directly into the 4K-capable framebuffer | M2 | ORIGINAL_REQUEST §R1 |
| 6 | Foreground Occlusion Mask Alignment | Screen-space UV mapping for depth-sorted mask quads over 3D actors, preventing pixelation and seams | M2 | ORIGINAL_REQUEST §R1 |
| 7 | xxHash64 Texture Fingerprinting | 64-bit deterministic hash computed from tpage, clut, palette, and VRAM texels for 3D actor meshes | M3 | ORIGINAL_REQUEST §R2 |
| 8 | HD Model Replacement Pipeline | Loading PNG replacements from `mods/textures/<pack>/replacements/<hash>.png` and binding in GL pipeline | M3 | ORIGINAL_REQUEST §R2 |
| 9 | UV Normalization for 3D Meshes | Convert PS1 0..255 vertex UVs to normalized [0.0, 1.0] coordinates with texel centering | M3 | ORIGINAL_REQUEST §R2 |
| 10 | Anisotropic & Trilinear Quality Filtering | Up to 16x `GL_TEXTURE_MAX_ANISOTROPY_EXT` and `GL_LINEAR_MIPMAP_LINEAR` filtering with mipmap generation | M3 | ORIGINAL_REQUEST §R2 |
| 11 | Transparent PS1 Texture Fallback | Zero-overhead fallback to original VRAM texture sampling when replacement is missing or pack is partial | M3 | ORIGINAL_REQUEST §R2 |
| 12 | HD UI 2D Screen Backdrops | 1080p/4K background replacement for inventory, status, map, epilogues, and clock from `mods/textures/<pack>/ui/` | M4 | ORIGINAL_REQUEST §R3 |
| 13 | HD UI Sprite & Icon Replacements | Hash-based replacement for individual UI icons, health bar, and fonts | M4 | ORIGINAL_REQUEST §R3 |
| 14 | Automated Texture Dumping Tooling | Export all decoded textures with hashes and metadata to PNG when `dump_textures = true` | M5 | ORIGINAL_REQUEST §R4 |
| 15 | Launcher "Mods & Texturas HD" Dialog | Win32 GUI dialog in `Resident_Evil_3_Launcher.exe` with pack selector, checkboxes, and Open Mods Folder button | M6 | ORIGINAL_REQUEST §R5 |
| 16 | Launcher Settings Persistence | Load and save `[textures]` section to `settings.toml` from launcher | M6 | ORIGINAL_REQUEST §R5 |
| 17 | E2E Regression & Quality Hardening | End-to-end verification across all tiers (Tiers 1-4) and adversarial stress-testing (Tier 5) | M7 | Acceptance Criteria |

---

## Milestones

| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Settings & Config Engine | Features 1. Shared `re3_textures.h` header, `settings.toml` parser extension in `main.cpp` and `config_loader.cpp` | None | IN_PROGRESS (worker: dfa10308) |
| M2 | HD Pre-rendered Backgrounds Engine | Features 2, 3, 4, 5, 6. `re3_hd_backgrounds.cpp/.h`, async prefetcher, RESHDP resolver, blit to `s_hr_fbo`, mask quad alignment | M1 | PLANNED |
| M3 | 3D Model & Actor HD Texture Subsystem | Features 7, 8, 9, 10, 11. `re3_hd_textures.cpp/.h`, `xxhash.h`, shader `TEX_FS` extension, UV normalization, aniso 16x, PS1 fallback | M1 | PLANNED |
| M4 | HD User Interface & Menus | Features 12, 13. `re3_hd_ui.cpp/.h`, menu quad interception, UI backdrops & sprite hash replacement | M1 | COMPLETED (worker: worker_m4_1) |
| M5 | Texture Dumping Tooling | Feature 14. PNG dump pipeline with session deduplication when `dump_textures = true` | M1, M3 | PLANNED |
| M6 | Win32 Launcher Interface | Features 15, 16. `launcher/main.cpp`, `ShowTexturesDialog`, directory enumeration, settings persistence | M1 | PLANNED |
| M7 | Final E2E Integration & Coverage Hardening | Feature 17. 100% pass on E2E test suite (Tiers 1-4) + Tier 5 adversarial coverage hardening | M2, M3, M4, M5, M6, E2E-TESTS | PLANNED |

---

## Interface Contracts

### `re3_textures.h` ↔ Subsystems
```c
#ifndef RE3_TEXTURES_H
#define RE3_TEXTURES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char pack[128];
    bool hd_backgrounds;
    bool hd_models;
    bool hd_ui;
    bool dump_textures;
} re3_texture_config_t;

extern re3_texture_config_t g_re3_tex_config;

void re3_textures_init(const char* base_path);
void re3_textures_shutdown(void);
const re3_texture_config_t* re3_textures_get_config(void);

#ifdef __cplusplus
}
#endif

#endif /* RE3_TEXTURES_H */
```

### `re3_hd_backgrounds.h` ↔ `gpu_gl_renderer.c`
```c
#ifndef RE3_HD_BACKGROUNDS_H
#define RE3_HD_BACKGROUNDS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void re3_hd_bg_init(const char* pack_path);
void re3_hd_bg_shutdown(void);
void re3_hd_bg_on_room_change(uint8_t stage, uint8_t room, uint8_t cut);
bool re3_hd_bg_render_current(uint32_t fbo, int width, int height);
bool re3_hd_bg_has_hd_current(void);
uint32_t re3_hd_bg_get_current_gl_tex(void);

#ifdef __cplusplus
}
#endif

#endif /* RE3_HD_BACKGROUNDS_H */
```

### `re3_hd_textures.h` ↔ `gpu_gl_renderer.c` / `gpu.c`
```c
#ifndef RE3_HD_TEXTURES_H
#define RE3_HD_TEXTURES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void re3_hd_tex_init(const char* pack_path);
void re3_hd_tex_shutdown(void);

uint64_t re3_hd_tex_compute_hash(uint16_t tpage, uint16_t clut, const uint8_t lim[4], const uint16_t* palette, int palette_len, const uint16_t* vram_pixels, int pixel_words);

uint32_t re3_hd_tex_bind_replacement(uint64_t hash); // returns GLuint or 0 for fallback

void re3_hd_tex_dump_if_enabled(uint64_t hash, uint16_t tpage, uint16_t clut, int w, int h, const uint32_t* rgba_pixels);

#ifdef __cplusplus
}
#endif

#endif /* RE3_HD_TEXTURES_H */
```

---

## Code Layout

- `psxrecomp/runtime/include/re3_textures.h`: Shared texture subsystem configuration and API contracts.
- `psxrecomp/runtime/include/xxhash.h`: Fast 64-bit hashing library (`XXH_INLINE_ALL`).
- `psxrecomp/runtime/src/re3_textures.cpp`: Configuration loading and state management.
- `psxrecomp/runtime/src/re3_hd_backgrounds.h / .cpp`: HD pre-rendered background loader, async prefetcher, RESHDP resolver, `s_hr_fbo` blitter.
- `psxrecomp/runtime/src/re3_hd_textures.h / .cpp`: 3D actor texture replacement, hash computation, GL texture cache, filtering setup, dumping.
- `psxrecomp/runtime/src/re3_hd_ui.h / .cpp`: HD 2D user interface backdrops and sprite replacement.
- `psxrecomp/runtime/src/gpu_gl_renderer.c`: Integration hooks in OpenGL pipeline, shader uniform `u_is_hd`, filtering configuration.
- `psxrecomp/runtime/src/gpu.c`: Primitive execution hooks for 3D actor tagging, UI detection, and dumping hooks.
- `psxrecomp/runtime/src/main.cpp`: Settings loading hook for `[textures]` in `settings.toml`.
- `launcher/main.cpp`: Win32 launcher GUI, `ShowTexturesDialog`, directory enumeration, settings persistence.
- `tests/e2e/`: E2E test suite runners, fixtures, and verification scripts.
