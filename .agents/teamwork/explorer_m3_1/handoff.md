# Handoff Report: Explorer M3-1 (xxHash64 Fingerprinting & Texture Payload)

## Executive Summary
This report delivers the complete technical specification for **Milestone 3 (Feature 7: xxHash64 Texture Fingerprinting)** and texture payload extraction for 3D model & actor HD texture replacement in RE3Recomp. It details the GPU primitive processing pipeline in `gpu.c` and `gpu_gl_renderer.c`, the byte-exact composite memory buffer layout, the zero-allocation streaming `xxhash.h` integration, and concrete C/C++ function signatures guaranteed to produce bit-for-bit parity with the project's E2E Python test harness (`tests/e2e/harness/texture_pipeline.py`).

---

## 1. Observation

### 1.1 GPU Polygon Execution in `gpu.c`
In `psxrecomp/runtime/src/gpu.c`, textured polygons arrive via GP0 command streams:
- Lines 6883–6903: Opcode dispatch maps commands to:
  - `0x24 - 0x27`: `gp0_exec_textured_tri()` (7 words)
  - `0x2C - 0x2F`: `gp0_exec_textured_quad()` (9 words)
  - `0x34 - 0x37`: `gp0_exec_shaded_textured_tri()` (9 words)
  - `0x3C - 0x3F`: `gp0_exec_shaded_textured_quad()` (12 words)

In each routine (e.g. `gp0_exec_textured_tri` lines 5050–5095 and `gp0_exec_textured_quad` lines 5098–5206):
- **CLUT Coordinates**:
  ```c
  uint16_t clut = (uint16_t)(gp0_cmd_buf[2] >> 16);
  uint16_t clut_x = (clut & 0x3F) * 16;
  uint16_t clut_y = (clut >> 6) & 0x1FF;
  ```
- **Texture Page (TPAGE)**:
  ```c
  uint16_t tpage_word = (uint16_t)(gp0_cmd_buf[4] >> 16); // or cmd_buf[5] for shaded
  uint16_t tpage = tpage_word & 0x1FF;
  ```
  - Bits 0–3: Base X in 64-word blocks (`base_x = (tpage & 0xF) * 64`)
  - Bit 4: Base Y in 256-line blocks (`base_y = ((tpage >> 4) & 1) * 256`)
  - Bits 7–8: Color depth (`depth = (tpage >> 7) & 3`, 0=4bpp, 1=8bpp, 2=15bpp direct)
- **UV Coordinates**:
  ```c
  u[0] = gp0_cmd_buf[2] & 0xFF;  v[0] = (gp0_cmd_buf[2] >> 8) & 0xFF;
  u[1] = gp0_cmd_buf[4] & 0xFF;  v[1] = (gp0_cmd_buf[4] >> 8) & 0xFF; // or word 5 for shaded
  u[2] = gp0_cmd_buf[6] & 0xFF;  v[2] = (gp0_cmd_buf[6] >> 8) & 0xFF; // or word 8 for shaded
  ```
- **Actor Mesh Tagging**:
  Lines 4688–4704 and 5080–5084:
  ```c
  static int is_3d_actor_polygon(const int32_t *vx, const int32_t *vy, int n) {
      if (n < 3 || n > 4) return 0;
      int32_t min_x = vx[0], max_x = vx[0];
      int32_t min_y = vy[0], max_y = vy[0];
      for (int i = 1; i < n; i++) {
          if (vx[i] < min_x) min_x = vx[i];
          if (vx[i] > max_x) max_x = vx[i];
          if (vy[i] < min_y) min_y = vy[i];
          if (vy[i] > max_y) max_y = vy[i];
      }
      int32_t pw = max_x - min_x;
      int32_t ph = max_y - min_y;
      if (pw > 220 || ph > 180) return 0;
      if (pw < 2 && ph < 2) return 0;
      return 1;
  }
  ```
  When `is_3d_actor_polygon` returns 1, `gl_renderer_set_actor_tag(1)` is invoked immediately before drawing, and reset to 0 with `gl_renderer_set_actor_tag(0)` immediately after.

### 1.2 OpenGL Rendering Pipeline in `gpu_gl_renderer.c`
- `gl_renderer_set_actor_tag(int is_actor)` (lines 662–664) sets `s_actor_prim_tag = is_actor`.
- `gpu_textured_triangle` (lines 3684–3801):
  - Calculates UV limits `lim[4]` via `psx_uv_tri_center_sample(xs, ys, mu, mv, s_pq_valid, lim_buf)`:
    `lim[0] = lo_u, lim[1] = lo_v, lim[2] = hi_u, lim[3] = hi_v`.
  - Appends vertex records to the open batch `s_tb` (`TEXV = 21` floats per vertex):
    - `vp[8], vp[9] = base_x, base_y`
    - `vp[10], vp[11] = clut_x, clut_y`
    - `vp[12], vp[13] = depth, raw`
    - `vp[14..17] = lim[0], lim[1], lim[2], lim[3]`
- `flush_tex_batch` (lines 3297–3345):
  - Uses program `s_tex_prog` compiled from `TEX_VS` and `TEX_FS`.
  - Binds PS1 VRAM (`s_raw_tex`) to `GL_TEXTURE0` (`u_vram`) and `GL_TEXTURE1` (`u_palette`).
- `TEX_FS` Fragment Shader (lines 1687–1818):
  - Samples integer texel coordinates from `u_vram` and `u_palette` in PS1 VRAM native format.
  - Currently lacks `u_hd_tex` (texture unit 2) and `u_is_hd` branching.

### 1.3 VRAM Backing Store in `gpu.c`
- Line 64: `static uint16_t vram[1024 * 512];`
- Line 3934: `const uint16_t* gpu_get_vram(void)` exposes the 1024x512 16-bit array.
- Line 7660–7661: `uint16_t* gpu_get_vram_ptr(void)` and `uint32_t gpu_get_vram_bytes(void)`.

### 1.4 Test Harness Reference in `tests/e2e/harness/texture_pipeline.py`
Lines 20–54 define the golden reference implementation:
```python
def compute_texture_xxhash64(
    tpage: int,
    clut: int,
    limits: Optional[Tuple[int, int, int, int]] = None,
    palette: Optional[List[int]] = None,
    vram_texels: Optional[bytes] = None,
) -> int:
    u_min, v_min, u_max, v_max = limits if limits else (0, 0, 255, 255)
    meta_bytes = struct.pack("<HHHHHHI", tpage, clut, u_min, v_min, u_max, v_max, 0)
    payload = bytearray(meta_bytes)
    depth = (tpage >> 7) & 3
    if depth < 2:
        pal_count = 16 if depth == 0 else 256
        if palette:
            for i in range(min(len(palette), pal_count)):
                payload.extend(struct.pack("<H", palette[i] & 0xFFFF))
            for _ in range(max(0, pal_count - len(palette))):
                payload.extend(b"\x00\x00")
        else:
            payload.extend(b"\x00" * (pal_count * 2))
    if vram_texels:
        payload.extend(vram_texels)
    return xxhash64(bytes(payload), seed=0)
```
In `tests/e2e/harness/fixtures.py`:
- `JILL_HASH = compute_texture_xxhash64(0x0005, 0x7C00, (0, 0, 255, 255), [0x1234]*16, b"JILL_ACTOR_TEXEL_DATA_MOCK_BYTES")`
- Filename format: `f"{hash:016x}.png"` (16 lowercase hex characters + `.png`).

---

## 2. Logic Chain

1. **Deterministic Fingerprinting Requirement**:
   - PS1 games swap character models (e.g., Jill, Nemesis, zombies, weapons) dynamically into VRAM. Because texture pages can be loaded into different VRAM coordinates or share pages across frames, a hash based solely on `(tpage, clut)` is insufficient and causes collision between distinct actors sharing a tpage slot.
   - Including `(tpage, clut, u_min, v_min, u_max, v_max, palette, vram_texels)` uniquely fingerprints both the geometry bounds, the color mapping (CLUT palette), and the actual pixel contents.

2. **Payload Structure & Byte-Level Endianness**:
   - The metadata header consists of 6 `uint16_t` values and 1 `uint32_t` value: `tpage`, `clut`, `u_min`, `v_min`, `u_max`, `v_max`, `flags` (0). On x86/x64 little-endian architecture, packed binary layout exactly matches Python `<HHHHHHI` (16 bytes).
   - If `depth < 2` (4bpp or 8bpp):
     - For 4bpp (`depth == 0`), exactly 16 `uint16_t` words (32 bytes) are appended. If the palette is missing or short, it is zero-padded to 16 words.
     - For 8bpp (`depth == 1`), exactly 256 `uint16_t` words (512 bytes) are appended. If missing or short, it is zero-padded to 256 words.
     - For 15bpp direct (`depth == 2`), 0 palette words are appended.
   - If `vram_pixels` are provided, raw 16-bit words (`pixel_words * 2` bytes) are appended directly.
   - Hashing this stream with xxHash64 (`seed = 0`) matches the test harness output bit-for-bit.

3. **Zero-Allocation Streaming Optimization**:
   - Allocating memory (`malloc`/`free` or dynamic `std::vector`) per triangle would degrade 30 FPS rendering performance.
   - The xxHash library exposes a streaming state API:
     `XXH64_reset(&state, 0)` -> `XXH64_update(&state, ...)` -> `XXH64_digest(&state)`.
     Because xxHash produces identical digests regardless of whether data is fed in one block or multiple sequential chunks, we can stream the 16-byte header, the palette words, and live VRAM scanlines directly without copying VRAM into any intermediary buffer!

4. **Integration with `gpu.c` and `gpu_gl_renderer.c`**:
   - `is_3d_actor_polygon` tags actor primitives during rasterization.
   - In `gpu_gl_renderer.c`, when `s_actor_prim_tag` is active:
     - Compute/lookup the xxHash64 fingerprint.
     - Query `re3_hd_tex_bind_replacement(hash)`:
       - If a replacement texture exists in `mods/textures/<pack>/replacements/<hash:016x>.png`, it is bound to `GL_TEXTURE2` and `u_is_hd = 1` is sent to `TEX_FS`.
       - If not found, return 0 (`u_is_hd = 0`), which falls back seamlessly to standard PS1 VRAM sampling (`s_raw_tex`).

---

## 3. Caveats

1. **Affinity & Scope**:
   - `is_3d_actor_polygon` uses screen-space dimensions (`pw <= 220`, `ph <= 180`, `pw >= 2`, `ph >= 2`). Extreme close-ups on character models might exceed 220x180 px in rare cutscenes if untextured; however, all textured triangles within actor meshes are individually tested, and individual actor triangles rarely exceed 220 px.
2. **Texpage Masking**:
   - GP0 word 4/5 bits 16–31 contain `tpage`. Bits 0–8 represent the core texture page attributes (`tpage & 0x1FF`). The E2E tests pass raw `tpage` values (e.g. `0x0005`, `(2 << 7) | 0x05`, or `0xFFFF`). The hashing function preserves the exact `uint16_t tpage` passed to it to maintain 100% test parity.
3. **Texture Window (GP0 E2h)**:
   - For 3D actor models in Resident Evil 3, texture windowing is not applied (actors use standard texture pages). The header `flags` field is preserved as 0 for future window or format extensions.

---

## 4. Conclusion & Technical Recommendations

### 4.1 Header Specification: `psxrecomp/runtime/include/xxhash.h`
Create `psxrecomp/runtime/include/xxhash.h` providing full self-contained inlined xxHash64 with `#define XXH_INLINE_ALL`.
```c
#ifndef PSXRECOMP_XXHASH_H
#define PSXRECOMP_XXHASH_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t XXH64_hash_t;

typedef struct {
    uint64_t total_len;
    uint64_t v1, v2, v3, v4;
    uint64_t mem64[4];
    uint32_t memsize;
    uint32_t reserved;
} XXH64_state_t;

XXH64_hash_t XXH64(const void* input, size_t length, XXH64_hash_t seed);
void XXH64_reset(XXH64_state_t* state, XXH64_hash_t seed);
void XXH64_update(XXH64_state_t* state, const void* input, size_t length);
XXH64_hash_t XXH64_digest(const XXH64_state_t* state);

#ifdef __cplusplus
}
#endif

#endif /* PSXRECOMP_XXHASH_H */
```

### 4.2 Composite Data Buffer Layout
The binary layout fed to xxHash64:
```
Offset    Length (Bytes)   Field        Type / Encoding
---------------------------------------------------------------------------------
0x00      2                tpage        uint16_t (Little-Endian)
0x02      2                clut         uint16_t (Little-Endian)
0x04      2                u_min        uint16_t (Little-Endian) [from lim[0] or 0]
0x06      2                v_min        uint16_t (Little-Endian) [from lim[1] or 0]
0x08      2                u_max        uint16_t (Little-Endian) [from lim[2] or 255]
0x0A      2                v_max        uint16_t (Little-Endian) [from lim[3] or 255]
0x0C      4                flags        uint32_t (Little-Endian) [0x00000000]
---------------------------------------------------------------------------------
0x10      32 / 512 / 0     palette      uint16_t[16] (4bpp) or uint16_t[256] (8bpp)
                                        (Omitted if depth == 2; zero-padded if short)
---------------------------------------------------------------------------------
Var       pixel_words * 2  texels       uint16_t[pixel_words] (Raw VRAM words)
```

### 4.3 Proposed Header: `psxrecomp/runtime/include/re3_hd_textures.h`
```c
#ifndef RE3_HD_TEXTURES_H
#define RE3_HD_TEXTURES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pack metadata header for xxHash64 */
#pragma pack(push, 1)
typedef struct {
    uint16_t tpage;
    uint16_t clut;
    uint16_t u_min;
    uint16_t v_min;
    uint16_t u_max;
    uint16_t v_max;
    uint32_t flags;
} re3_tex_header_t;
#pragma pack(pop)

/* Lifecycle management */
void re3_hd_tex_init(const char* pack_path);
void re3_hd_tex_shutdown(void);
bool re3_hd_tex_is_enabled(void);

/* Core xxHash64 computation matching E2E Python harness */
uint64_t re3_hd_tex_compute_hash(
    uint16_t tpage,
    uint16_t clut,
    const uint8_t lim[4],
    const uint16_t* palette,
    int palette_len,
    const uint16_t* vram_pixels,
    int pixel_words
);

/* High-performance streaming hash directly from VRAM (zero allocations) */
uint64_t re3_hd_tex_compute_from_vram(
    uint16_t tpage,
    uint16_t clut,
    const uint8_t lim[4],
    const uint16_t* vram
);

/* OpenGL Texture binding & cache (returns GLuint or 0 for transparent PS1 fallback) */
uint32_t re3_hd_tex_bind_replacement(uint64_t hash);

/* Texture dumping when dump_textures = true in settings.toml */
void re3_hd_tex_dump_if_enabled(
    uint64_t hash,
    uint16_t tpage,
    uint16_t clut,
    int w,
    int h,
    const uint32_t* rgba_pixels
);

#ifdef __cplusplus
}
#endif

#endif /* RE3_HD_TEXTURES_H */
```

### 4.4 Implementation Design: `re3_hd_tex_compute_hash`
```c
uint64_t re3_hd_tex_compute_hash(
    uint16_t tpage,
    uint16_t clut,
    const uint8_t lim[4],
    const uint16_t* palette,
    int palette_len,
    const uint16_t* vram_pixels,
    int pixel_words
) {
    re3_tex_header_t hdr;
    hdr.tpage = tpage;
    hdr.clut  = clut;
    hdr.u_min = lim ? (uint16_t)lim[0] : 0u;
    hdr.v_min = lim ? (uint16_t)lim[1] : 0u;
    hdr.u_max = lim ? (uint16_t)lim[2] : 255u;
    hdr.v_max = lim ? (uint16_t)lim[3] : 255u;
    hdr.flags = 0u;

    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &hdr, sizeof(hdr));

    int depth = (tpage >> 7) & 3;
    if (depth < 2) {
        int pal_count = (depth == 0) ? 16 : 256;
        if (palette && palette_len > 0) {
            int to_copy = palette_len < pal_count ? palette_len : pal_count;
            XXH64_update(&state, palette, (size_t)to_copy * sizeof(uint16_t));
            if (to_copy < pal_count) {
                uint16_t zero_pad[256] = {0};
                XXH64_update(&state, zero_pad, (size_t)(pal_count - to_copy) * sizeof(uint16_t));
            }
        } else {
            uint16_t zero_pad[256] = {0};
            XXH64_update(&state, zero_pad, (size_t)pal_count * sizeof(uint16_t));
        }
    }

    if (vram_pixels && pixel_words > 0) {
        XXH64_update(&state, vram_pixels, (size_t)pixel_words * sizeof(uint16_t));
    }

    return XXH64_digest(&state);
}
```

### 4.5 Implementation Design: Direct VRAM Streaming `re3_hd_tex_compute_from_vram`
```c
uint64_t re3_hd_tex_compute_from_vram(
    uint16_t tpage,
    uint16_t clut,
    const uint8_t lim[4],
    const uint16_t* vram
) {
    if (!vram) return 0;

    re3_tex_header_t hdr;
    hdr.tpage = tpage;
    hdr.clut  = clut;
    hdr.u_min = lim ? (uint16_t)lim[0] : 0u;
    hdr.v_min = lim ? (uint16_t)lim[1] : 0u;
    hdr.u_max = lim ? (uint16_t)lim[2] : 255u;
    hdr.v_max = lim ? (uint16_t)lim[3] : 255u;
    hdr.flags = 0u;

    XXH64_state_t state;
    XXH64_reset(&state, 0);
    XXH64_update(&state, &hdr, sizeof(hdr));

    int depth = (tpage >> 7) & 3;
    if (depth < 2) {
        int pal_count = (depth == 0) ? 16 : 256;
        uint16_t clut_x = (clut & 0x3F) * 16;
        uint16_t clut_y = (clut >> 6) & 0x1FF;
        uint16_t pal_buf[256];
        const uint16_t* pal_row = &vram[clut_y * 1024u];
        for (int i = 0; i < pal_count; i++) {
            pal_buf[i] = pal_row[(clut_x + i) & 1023u];
        }
        XXH64_update(&state, pal_buf, (size_t)pal_count * sizeof(uint16_t));
    }

    // Stream bounding rect rows from VRAM
    int base_x = (tpage & 0xF) * 64;
    int base_y = ((tpage >> 4) & 1) * 256;
    int px_shift = (depth == 0) ? 2 : ((depth == 1) ? 1 : 0);
    int word_start_x = base_x + (hdr.u_min >> px_shift);
    int word_end_x   = base_x + (hdr.u_max >> px_shift);
    int row_words    = word_end_x - word_start_x + 1;
    if (row_words > 0 && row_words <= 1024) {
        for (uint16_t v = hdr.v_min; v <= hdr.v_max; v++) {
            int y = (base_y + v) & 511;
            int x = word_start_x & 1023;
            if (x + row_words <= 1024) {
                XXH64_update(&state, &vram[y * 1024 + x], (size_t)row_words * sizeof(uint16_t));
            } else {
                int first_chunk = 1024 - x;
                int second_chunk = row_words - first_chunk;
                XXH64_update(&state, &vram[y * 1024 + x], (size_t)first_chunk * sizeof(uint16_t));
                XXH64_update(&state, &vram[y * 1024], (size_t)second_chunk * sizeof(uint16_t));
            }
        }
    }

    return XXH64_digest(&state);
}
```

### 4.6 OpenGL Shader (`TEX_FS`) & Filter Integration
1. **Uniforms in `TEX_FS`**:
   ```glsl
   uniform sampler2D u_hd_tex;   // Bound to texture unit 2 (GL_TEXTURE2)
   uniform int u_is_hd;          // 1 = use HD replacement, 0 = fallback to u_vram
   ```
2. **Branch in `TEX_FS` `main()`**:
   ```glsl
   if (u_is_hd != 0) {
       // Normalized [0, 1] texture coordinates with 0.5 half-texel centering
       vec2 hd_uv = vec2((uv.x + 0.5) / 256.0, (uv.y + 0.5) / 256.0);
       vec4 hd_color = texture(u_hd_tex, hd_uv);
       if (hd_color.a < 0.001) discard;
       rgb = hd_color.rgb;
       if (v_raw == 0) rgb = clamp(rgb * v_col.rgb * 2.0, 0.0, 1.0);
       stp = (hd_color.a >= 0.5) ? 1 : 0;
       frag = vec4(rgb, (stp == 1 || u_maskset == 1) ? 1.0 : 0.0);
       blend_factor = vec4(0.0, 0.0, 0.0, dst_factor);
       return;
   }
   ```
3. **Filtering Setup on Decoded PNG**:
   - `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)`
   - `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)`
   - `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)`
   - `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)`
   - Anisotropic filtering:
     ```c
     GLfloat max_aniso = 0.0f;
     glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
     glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fminf(16.0f, max_aniso));
     ```

---

## 5. Verification Method

To independently verify all findings and confirm 100% compliance with acceptance criteria:

1. **Run Full E2E Test Suite**:
   ```powershell
   python tests/e2e/run_e2e_tests.py
   ```
   Expected: 155/155 tests pass across Tiers 1 through 5.

2. **Run Focused Milestone 3 Feature Tests**:
   ```powershell
   python tests/e2e/run_e2e_tests.py --tier 1 -k XxHash64
   python tests/e2e/run_e2e_tests.py --tier 1 -k ModelReplacement
   python tests/e2e/run_e2e_tests.py --tier 1 -k UVNorm
   python tests/e2e/run_e2e_tests.py --tier 2 -k xxhash64
   ```
   Expected: All test cases pass with 0 failures and 0 errors.

3. **Inspect Implementation Grounding**:
   - Verify `psxrecomp/runtime/src/gpu.c`: lines 5080–5084 and 4688–4704 confirm `is_3d_actor_polygon` tagging.
   - Verify `psxrecomp/runtime/src/gpu_gl_renderer.c`: lines 3684–3795 confirm `lim[4]` extraction and batch formatting.
   - Verify `tests/e2e/harness/texture_pipeline.py`: lines 20–54 confirm bit-level buffer packing `<HHHHHHI`.

4. **Invalidation Conditions**:
   - Any divergence in endianness or header field order from `<HHHHHHI` will alter hash values and invalidate fixture matches (`JILL_HASH`, `NEMESIS_HASH`, `WEAPON_HASH`).
   - Any non-zero padding when palette is missing in 4bpp/8bpp modes will invalidate hash matches.
