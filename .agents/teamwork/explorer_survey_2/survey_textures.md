# Technical Survey: 3D Model & Actor HD Texture Replacement (R2) and Texture Dumping Tooling (R4)

**Project**: RE3Recomp (Resident Evil 3: Nemesis Recompiled / Project RE: Visited)  
**Agent**: Explorer 2 (3D Texture & Dumping Explorer)  
**Date**: 2026-10-18  
**Scope**: Requirements R2 & R4 (PS1 Graphics Pipeline, xxHash64 Fingerprinting, HD Replacement Binding, UV Normalization, Texture Filtering & Mipmapping, Texture Cache/Fallback, Dumping Tooling)

---

## 1. Executive Summary

This survey provides a comprehensive technical investigation into the graphics architecture of RE3Recomp to implement **Requirement R2** (3D Model & Actor HD Texture Replacement) and **Requirement R4** (Texture Dumping Tooling).

The investigation revealed that:
1. **Graphics Pipeline**: RE3Recomp translates PS1 GTE-transformed 3D polygons into GP0 commands (`0x24`/`0x25`, `0x2C`/`0x2D`, `0x34`/`0x36`, `0x3C`/`0x3E`) in `psxrecomp/runtime/src/gpu.c`. 3D actor geometry is already detected via `is_3d_actor_polygon()` (`gpu.c:4688`) and tagged via `gl_renderer_set_actor_tag(1)` (`gpu_gl_renderer.c:661`).
2. **PS1 Texture Decoding**: In the OpenGL backend (`gpu_gl_renderer.c`), VRAM is maintained as a single 1024x512 16-bit integer texture (`s_raw_tex`, `GL_R16UI`). Fragment shader `TEX_FS` dynamically decodes 4-bit, 8-bit, and 15-bit CLUT textures on-the-fly inside `fetch_texel()` using `u_vram` and `u_palette`.
3. **Existing HD Subsystems**: An unwired, highly sophisticated Beetle-style texture pack module exists in `psxrecomp/runtime/src/hd_texture_pack.cpp` and `include/hd_texture_pack.h`, alongside a texture bank mechanism in `mod_texture_banks.c`. However, neither is hooked into live 3D actor rendering.
4. **Fingerprinting**: `xxHash64` is not currently in the repository (only CRC32 in `crc32.h` and FNV-1a/splitmix64 in `frame_fingerprint.h`). It can be added as a zero-dependency header (`third_party/xxhash/xxhash.h` with `XXH_INLINE_ALL`). The 64-bit fingerprint must hash metadata (`tpage`, `clut`, limits), the CLUT palette words, and the VRAM texel data.
5. **UV Normalization**: Original PS1 UVs (0..255) must be normalized to `[0.0, 1.0]` for `GL_TEXTURE_2D`. Mathematical mapping depends on whether the replacement represents the full 256x256 texture page or an authored bounding sub-rectangle.
6. **Filtering & Mipmaps**: `GL_LINEAR_MIPMAP_LINEAR` + `GL_LINEAR` with `glGenerateMipmap` and `GL_TEXTURE_MAX_ANISOTROPY_EXT` (up to 16x) can be cleanly enabled on modern OpenGL contexts.
7. **Dumping**: `dump_textures = true` can be parsed in `load_re3_qol_settings()` (`main.cpp:5691`), hooked in `gpu.c` / `gpu_gl_renderer.c`, and exported as PNGs via `stbi_write_png` (or the existing Deflate chunk encoder in `test_hd_texture_pack.cpp`).

---

## 2. RE3Recomp 3D Polygon Texture Pipeline (Question 1)

### 2.1 Call Flow from Game Engine to OpenGL

The complete execution chain for 3D polygon drawing in RE3Recomp runs as follows:

```
[Resident Evil 3 Game Engine (SLUS-009.23)]
   │
   ├─ GTE transforms actor vertices (Jill, Nemesis, enemies, weapons)
   │
   ▼
[psxrecomp/runtime/src/gpu.c]
   ├─ GP0 Command Dispatcher:
   │    • 0x24 / 0x25 -> gp0_exec_textured_tri()       (line 5050)
   │    • 0x2C / 0x2D -> gp0_exec_textured_quad()      (line 5098)
   │    • 0x34 / 0x36 -> gp0_exec_shaded_textured_tri() (line 5209)
   │    • 0x3C / 0x3E -> gp0_exec_shaded_textured_quad() (line 5284)
   │
   ├─ 3D Actor Detection & Tagging:
   │    • is_3d_actor_polygon(vx, vy, 3) (line 4688)
   │    • gl_renderer_set_actor_tag(1)   (line 5083, 5244)
   │
   ├─ Perspective & Precise Setup:
   │    • prepare_precise_triangle()
   │    • prepare_texture_triangle()
   │
   ▼
[psxrecomp/runtime/src/gpu_render.c] (Renderer Facade)
   ├─ gr_draw_textured_triangle()        (line 125)
   ├─ gr_draw_shaded_textured_triangle() (line 131)
   │
   ▼
[psxrecomp/runtime/src/gpu_gl_renderer.c] (OpenGL Backend)
   ├─ glb_draw_textured_triangle()        (line 4181)
   ├─ glb_draw_shaded_textured_triangle() (line 4191)
   │    │
   │    ▼
   ├─ gpu_textured_triangle()             (line 3681)
   │    • Batches into s_tb[TEXBATCH_MAXV * TEXV]
   │    • Records actor geometry in actor_geo_record_triangle() (line 3792)
   │
   ▼
   └─ flush_tex_batch()                   (line 3294)
        • Binds s_tex_prog
        • Binds s_raw_tex (GL_R16UI) to GL_TEXTURE0 (u_vram) and GL_TEXTURE1 (u_palette)
        • Uploads vertex data to s_tex_vbo
        • Calls tex_batch_draw_passes(nverts, semi) -> glDrawArrays()
```

### 2.2 Actor Identification Mechanism

RE3Recomp already includes a bounding-box heuristic to identify 3D character and creature polygons:
- **Location**: `psxrecomp/runtime/src/gpu.c:4688`
- **Function**: `static int is_3d_actor_polygon(const int32_t *vx, const int32_t *vy, int n)`
  - Calculates polygon width `pw = max_x - min_x` and height `ph = max_y - min_y`.
  - Rejects screen-filling / background quads: `if (pw > 220 || ph > 180) return 0;`.
  - Rejects degenerate/sub-pixel noise: `if (pw < 2 && ph < 2) return 0;`.
  - Sets tag: `gl_renderer_set_actor_tag(1)` in `gpu_gl_renderer.c:661`.

In addition, room and camera cut context is actively tracked in `gpu_gl_renderer.c:805`:
- `uint8_t cur_room = psx_read_byte(0x000D1F09);`
- `uint8_t cur_cut  = psx_read_byte(0x000D1F0A);`
- `uint8_t cur_stage = psx_read_byte(0x000D1F08);`

---

## 3. PS1 VRAM/tpage/clut Conversion in OpenGL (Question 2)

### 3.1 Architecture of PS1 Texture Representation

Unlike modern engines where each surface or sprite has a standalone texture, the PS1 architecture stores all graphics in a contiguous **1024 x 512 16-bit VRAM**:
- In `gpu_gl_renderer.c`, this is mirrored on the GPU as `s_raw_tex` (`GL_R16UI`, 1024x512, format `GL_RED_INTEGER`, type `GL_UNSIGNED_SHORT` at line 4761).
- Rendering targets the high-resolution FBO `s_hr_tex` (`GL_RGBA8`, `1024*S x 512*S`, where `S` is the supersampling scale).
- Whenever CPU uploads pixel data (GP0 command `0xA0`, `gp0_exec_cpu_to_vram()`), pixels are staged and updated in `s_raw_tex` via `glTexSubImage2D()` (`gpu_gl_renderer.c:2051`).

### 3.2 Dynamic Shader CLUT Decode (`TEX_FS`)

In `psxrecomp/runtime/src/gpu_gl_renderer.c:1684-1815`, `TEX_FS` performs real-time CLUT decoding in the fragment shader:

```glsl
int fetch_texel(int u, int v) {
  u &= 255; v &= 255;
  // Texture window GP0(E2h) masking and clamp to v_limits
  ...
  if (v_depth == 0) { // 4-bit indexed (16 colors)
    int px = vram_at(v_tpage.x + (u >> 2), v_tpage.y + v);
    return palette_at(v_clut.x + ((px >> ((u & 3) * 4)) & 0xF), v_clut.y);
  } else if (v_depth == 1) { // 8-bit indexed (256 colors)
    int px = vram_at(v_tpage.x + (u >> 1), v_tpage.y + v);
    return palette_at(v_clut.x + ((px >> ((u & 1) * 8)) & 0xFF), v_clut.y);
  }
  return vram_at(v_tpage.x + u, v_tpage.y + v); // 15-bit direct
}
```

### 3.3 What Must Change for HD Replacement Textures

When an HD replacement texture is active:
1. The texture is **not** an R16UI index map — it is a standard 32-bit `GL_RGBA8` image (e.g., 2048x2048).
2. The shader must bypass `fetch_texel()` and sample `texture(u_hd_tex, v_hd_uv)` directly.
3. Vertex color modulation (`v_col`), semi-transparency blending (`u_semimode`), and mask bit testing (`u_maskset`) must remain active so lighting and transparency match original game behavior.

---

## 4. 64-bit xxHash64 Fingerprint Calculation (Question 3)

### 4.1 Current Repository Status

- `xxHash` is **not** present in the repository.
- Existing hashing tools in the repo:
  - `psxrecomp/runtime/include/crc32.h`: IEEE 802.3 32-bit CRC.
  - `psxrecomp/runtime/include/frame_fingerprint.h`: FNV-1a 64-bit + splitmix64 for CPU write tracking.
- `xxHash` (`xxhash.h`) is standard BSD-2-Clause open source, header-only via `#define XXH_INLINE_ALL`. Adding `psxrecomp/runtime/third_party/xxhash/xxhash.h` is lightweight and zero-dependency.

### 4.2 Fingerprint Specification for PS1 Textures

To uniquely identify each 3D model texture without false collisions or stutters:

#### Metadata Component (16 bytes)
```c
struct TextureFingerprintKey {
    uint16_t tpage;        /* Texture page word (base_x, base_y, depth) */
    uint16_t clut;         /* CLUT word (clut_x, clut_y) */
    uint16_t u_min, v_min; /* Texture bounding limits */
    uint16_t u_max, v_max;
    uint32_t flags;        /* Reserved / format flags */
};
```

#### Palette Component
- For 4-bit depth (`v_depth == 0`): 16 `uint16_t` halfwords (32 bytes) from VRAM at `(clut_x, clut_y)`.
- For 8-bit depth (`v_depth == 1`): 256 `uint16_t` halfwords (512 bytes) from VRAM at `(clut_x, clut_y)`.
- For 15-bit depth (`v_depth == 2`): 0 bytes (no CLUT).

#### Texel Data Component
- The raw halfwords from `s_vram` covering the bounding rectangle `[u_min..u_max, v_min..v_max]`.
- For typical actor sheets in RE3 (256x256 4bpp), the page is 64x256 halfwords = 32,768 bytes.

#### Hash Computation
```c
uint64_t compute_texture_xxhash64(uint16_t tpage, uint16_t clut,
                                  const int *limits, const uint16_t *vram) {
    XXH64_state_t* state = XXH64_createState();
    XXH64_reset(state, 0);

    // 1. Metadata
    struct TextureFingerprintKey key;
    key.tpage = tpage;
    key.clut  = clut;
    key.u_min = limits ? limits[0] : 0;
    key.v_min = limits ? limits[1] : 0;
    key.u_max = limits ? limits[2] : 255;
    key.v_max = limits ? limits[3] : 255;
    key.flags = 0;
    XXH64_update(state, &key, sizeof(key));

    // 2. Palette
    uint8_t depth = (tpage >> 7) & 3;
    if (depth < 2) {
        uint16_t clut_x = (clut & 0x3F) * 16;
        uint16_t clut_y = (clut >> 6) & 0x1FF;
        size_t pal_words = (depth == 0) ? 16 : 256;
        const uint16_t *pal = &vram[clut_y * 1024 + clut_x];
        XXH64_update(state, pal, pal_words * sizeof(uint16_t));
    }

    // 3. Texel data
    int base_x = (tpage & 0xF) * 64;
    int base_y = ((tpage >> 4) & 1) * 256;
    int w_words = (key.u_max - key.u_min + 1) / (depth == 0 ? 4 : (depth == 1 ? 2 : 1));
    int h_rows  = (key.v_max - key.v_min + 1);
    for (int r = 0; r < h_rows; r++) {
        const uint16_t *row = &vram[(base_y + key.v_min + r) * 1024 + (base_x + key.u_min / 4)];
        XXH64_update(state, row, w_words * sizeof(uint16_t));
    }

    uint64_t hash = XXH64_digest(state);
    XXH64_freeState(state);
    return hash;
}
```

Formatting: 16 lowercase hex characters (e.g. `snprintf(name, sizeof(name), "%016llx.png", (unsigned long long)hash)`).

---

## 5. UV Normalization & Mapping (Question 4)

### 5.1 Coordinate Spaces

| Coordinate Space | Range | Description |
|---|---|---|
| **PS1 Vertex UV** | `0 .. 255` | Integer coordinates within the 256x256 texture page |
| **OpenGL Standard UV** | `0.0 .. 1.0` | Normalized texture coordinates for `GL_TEXTURE_2D` |
| **HD Replacement Texture** | `W_hd x H_hd` | E.g., 2048x2048, replacing the 256x256 page |

### 5.2 Mathematical UV Mapping Formula

#### Scenario A: Full Texture Page Replacement (Standard for Character Models in RE3)
In RE3, actor models (Jill, Nemesis, Carlos, zombies, weapons) have TIM texture files that align to a 256x256 (or 128x256) page. HD packs replace the entire sheet with a high-resolution image (e.g. 2048x2048).
For vertex $i$ with coordinates $(u_i, v_i)$:

$$u_{\text{norm}} = \frac{u_i + 0.5}{256.0}$$

$$v_{\text{norm}} = \frac{v_i + 0.5}{256.0}$$

*(Note: Adding $0.5 / W$ aligns the PS1 integer texel centers to OpenGL normalized texel centers, eliminating 1-pixel edge bleeding).*

#### Scenario B: Sub-Rectangle Replacement
If an HD replacement texture corresponds to a bounded sub-rectangle defined by `lim = [u_min, v_min, u_max, v_max]`:

$$u_{\text{norm}} = \frac{u_i - u_{\text{min}}}{u_{\text{max}} - u_{\text{min}} + 1}$$

$$v_{\text{norm}} = \frac{v_i - v_{\text{min}}}{v_{\text{max}} - v_{\text{min}} + 1}$$

### 5.3 Perspective Correction

In `gpu_gl_renderer.c:1677-1683`, perspective-correct texturing is handled using hyperbolic $W$-division (`w = 1.0 / a_q`):
- `v_hd_uv` must be declared as `smooth out vec2 v_hd_uv;` when `v_persp != 0`, and `noperspective out vec2 v_hd_uv;` when `v_persp == 0`.
- This ensures that HD actor textures respect the game's perspective texturing setting (`[video] perspective_texturing = true`).

---

## 6. Texture Filtering & Anisotropic Mipmapping (Question 5)

### 6.1 OpenGL Setup Requirements

To deliver the visual quality specified in Acceptance Criteria (no UV distortion, no shimmering, stable 30 FPS):

```c
// 1. Minification & Magnification Filters
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Trilinear
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                 // Bilinear
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

// 2. Anisotropic Filtering (up to 16x)
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

GLfloat max_aniso = 1.0f;
glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
if (max_aniso > 1.0f) {
    GLfloat target_aniso = (max_aniso < 16.0f) ? max_aniso : 16.0f;
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, target_aniso);
}

// 3. Mipmap Generation
p_glGenerateMipmap(GL_TEXTURE_2D);
```

### 6.2 Function Pointer Loading

In `gpu_gl_renderer.c:310` (`load_modern_gl()`):
- Add `LOAD(p_glGenerateMipmap, "glGenerateMipmap");`.
- `p_glGenerateMipmap` is standard in OpenGL 3.0+ and Core Profile 3.3.

---

## 7. Texture Cache & Transparent Fallback Architecture (Question 6)

### 7.1 Architecture Design

```
                     [gp0_exec_textured_*]
                               │
                               ▼
               [Check 64-bit Hash in HD Texture Cache]
                               │
                 ┌─────────────┴─────────────┐
                 ▼                           ▼
        [Replacement Found]         [No Replacement]
                 │                           │
         Is GLuint loaded?                   │
         ┌───────┴───────┐                   │
         ▼               ▼                   │
       [Yes]           [No]                  │
         │          Load PNG,                │
         │        Create Mipmaps,            │
         │        Store in Cache             │
         │               │                   │
         └───────┬───────┘                   │
                 ▼                           ▼
         [HD Texture Path]           [Original PS1 Path]
         u_is_hd = 1                 u_is_hd = 0
         Bind u_hd_tex (GL_TEXTURE2) Bind s_raw_tex (GL_TEXTURE0)
         Normalized UVs              fetch_texel() from VRAM
```

### 7.2 Data Structures for Texture Management

```cpp
struct HdTextureEntry {
    GLuint gl_tex_id = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint64_t last_used_frame = 0;
    bool is_valid = false;
};

class HdTextureCache {
public:
    std::unordered_map<uint64_t, HdTextureEntry> m_entries;
    std::unordered_set<uint64_t> m_negative_cache; // Missing files (prevents disk hammering)
    std::string m_pack_path;
    size_t m_memory_budget_bytes = 256 * 1024 * 1024; // 256 MB default
    size_t m_current_memory_bytes = 0;

    GLuint get_or_load(uint64_t hash);
    void evict_lru();
    void reset();
};
```

### 7.3 Fallback Guarantee

- If a texture hash is absent from `m_entries` and cannot be found on disk:
  - Add to `m_negative_cache`.
  - Draw triangle using `u_is_hd = 0`.
  - The fragment shader runs its existing `fetch_texel()` code without changes.
  - **Zero visual glitch**: characters without HD textures continue to render identically to original PS1.

---

## 8. Texture Dumping Tooling (Question 7 - R4)

### 8.1 Configuration Hook

- Configuration file: `settings.toml` and `game.toml`
  ```toml
  [textures]
  pack = "seamless_hd"
  hd_backgrounds = true
  hd_models = true
  hd_ui = true
  dump_textures = false # set to true for tooling
  dump_dir = "dump/textures"
  ```
- Reading: Hooked in `main.cpp:5691` (`load_re3_qol_settings()`):
  ```cpp
  else if (key == "dump_textures") g_dump_textures = (val == "true" || val == "1");
  ```

### 8.2 Dumping Implementation

When `g_dump_textures == true`:
1. **Deduplication Set**: Maintain an in-memory set `static std::unordered_set<uint64_t> s_dumped_hashes;`.
2. **Hook Location**: In `gpu_gl_renderer.c:3710` (`gpu_textured_triangle()`) or `gpu.c`:
   - Compute `hash = compute_texture_xxhash64(...)`.
   - If `s_dumped_hashes.find(hash) == s_dumped_hashes.end()`:
     - Insert `hash` into `s_dumped_hashes`.
     - Decode the texture from `s_vram` with the current CLUT into an RGBA8 buffer `std::vector<uint32_t> rgba(w * h)`.
     - Call PNG writer:
       ```c
       char filename[512];
       snprintf(filename, sizeof(filename), "%s/%016llx.png", g_dump_dir.c_str(), (unsigned long long)hash);
       stbi_write_png(filename, w, h, 4, rgba.data(), w * 4);
       ```
3. **PNG Writing Tool**:
   - `stb_image.h` is already in `psxrecomp/runtime/third_party/stb_image.h`.
   - Add `stb_image_write.h` to `psxrecomp/runtime/third_party/` or use the self-contained uncompressed PNG generator demonstrated in `psxrecomp/runtime/tests/test_hd_texture_pack.cpp:75-115`.

---

## 9. Implementation Roadmap & Architecture Recommendations

| Step | Component | Target Files | Description |
|---|---|---|---|
| **1** | **xxHash64 Integration** | `runtime/third_party/xxhash/xxhash.h` | Add single-header `xxhash.h` with `XXH_INLINE_ALL` |
| **2** | **Texture Fingerprinting** | `runtime/src/hd_texture_hash.c`, `include/hd_texture_hash.h` | Implement `compute_texture_xxhash64()` based on tpage, clut, limits, and VRAM pixels |
| **3** | **GL Proc Loading** | `runtime/src/gpu_gl_renderer.c` | Add `p_glGenerateMipmap` to `load_modern_gl()` |
| **4** | **Shader Updates** | `runtime/src/gpu_gl_renderer.c` (`TEX_VS`, `TEX_FS`) | Add `u_is_hd`, `u_hd_tex`, and `v_hd_uv` support |
| **5** | **Texture Cache & Loader** | `runtime/src/hd_texture_cache.cpp`, `include/hd_texture_cache.h` | Implement `HdTextureCache` with anisotropy (16x), mipmaps, LRU eviction, and negative caching |
| **6** | **Pipeline Hooking** | `runtime/src/gpu_gl_renderer.c` (`gpu_textured_triangle`) | Hook replacement check, bind HD texture if present, calculate normalized UVs, fall back if absent |
| **7** | **Texture Dumping** | `runtime/src/hd_texture_dump.cpp`, `include/hd_texture_dump.h` | Implement `dump_textures = true` saving PNGs via `stbi_write_png` with deduplication |

---

*Report prepared by Explorer 2 (3D Texture & Dumping Explorer) for the RE3Recomp Team.*
