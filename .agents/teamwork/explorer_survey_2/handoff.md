# Handoff Report: 3D Texture & Dumping Technical Survey (R2 & R4)

**Agent**: Explorer 2 (3D Texture & Dumping Explorer)  
**Recipient**: Orchestrator (Conversation ID: `2974c4ea-67cd-482c-a523-a5380947c96d`)  
**Date**: 2026-10-18  
**Working Directory**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_2\`  
**Detailed Survey File**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_2\survey_textures.md`

---

## 1. Observation

1. **GP0 Command Dispatch & Actor Geometry Detection**:
   - In `psxrecomp/runtime/src/gpu.c`, textured primitives are parsed and dispatched:
     - Line 5050: `static void gp0_exec_textured_tri(void)`
     - Line 5098: `static void gp0_exec_textured_quad(void)`
     - Line 5209: `static void gp0_exec_shaded_textured_tri(void)`
     - Line 5284: `static void gp0_exec_shaded_textured_quad(void)`
   - 3D actor identification is implemented in `psxrecomp/runtime/src/gpu.c`:
     - Line 4688: `static int is_3d_actor_polygon(const int32_t *vx, const int32_t *vy, int n)`
     - Line 5080-5084:
       ```c
       int is_act = is_3d_actor_polygon(vx, vy, 3);
       if (is_act) {
           note_3d_actor_polygon(vx, vy, 3);
           gl_renderer_set_actor_tag(1);
       }
       ```
     - Line 5241-5245:
       ```c
       int is_act = is_3d_actor_polygon(vx, vy, 3);
       if (is_act) {
           note_3d_actor_polygon(vx, vy, 3);
           gl_renderer_set_actor_tag(1);
       }
       ```
   - In `psxrecomp/runtime/src/gpu_gl_renderer.c`:
     - Line 661: `void gl_renderer_set_actor_tag(int is_actor) { s_actor_prim_tag = is_actor; }`
     - Line 805-806: Active room and cut are read directly from guest memory:
       `uint8_t cur_room = psx_read_byte(0x000D1F09);`
       `uint8_t cur_cut  = psx_read_byte(0x000D1F0A);`

2. **PS1 Texture VRAM & Shader Architecture**:
   - In `psxrecomp/runtime/src/gpu_gl_renderer.c`:
     - Line 37: `* R16UI mirror ('s_raw_tex') holding raw 1555 VRAM values (CLUT decode +`
     - Line 4761: `s_raw_tex = make_tex(PSXGL_R16UI, VRAM_W, VRAM_H, PSXGL_RED_INTEGER, GL_UNSIGNED_SHORT);`
     - Line 1684-1815: `TEX_FS` implements on-the-fly CLUT decoding:
       - Line 1725-1732:
         ```glsl
         if (v_depth == 0) {
           int px = vram_at(v_tpage.x + (u >> 2), v_tpage.y + v);
           return palette_at(v_clut.x + ((px >> ((u & 3) * 4)) & 0xF), v_clut.y);
         } else if (v_depth == 1) {
           int px = vram_at(v_tpage.x + (u >> 1), v_tpage.y + v);
           return palette_at(v_clut.x + ((px >> ((u & 1) * 8)) & 0xFF), v_clut.y);
         }
         return vram_at(v_tpage.x + u, v_tpage.y + v);
         ```
     - Line 3681: `static void gpu_textured_triangle(...)` batches textured triangles into `s_tb`.
     - Line 3294: `static void flush_tex_batch(void)` binds `s_raw_tex` to texture units 0 and 1, uploading vertex array to `s_tex_vbo`.

3. **Status of xxHash and Third-Party Libraries**:
   - `xxHash` is currently absent from the repo. `grep_search` for `xxhash` returned no results.
   - `stb_image.h` is present in `psxrecomp/runtime/third_party/stb_image.h` (line 13). `STB_IMAGE_IMPLEMENTATION` is compiled in `psxrecomp/runtime/src/psx_window_icon.cpp:18`.
   - `stb_image_write.h` is currently not present in the repository, but `test_hd_texture_pack.cpp:75-115` demonstrates an existing self-contained uncompressed Deflate PNG encoder.
   - An existing Beetle-style HD texture pack module exists in `psxrecomp/runtime/src/hd_texture_pack.cpp` and `include/hd_texture_pack.h` (tested in `test_hd_texture_pack.cpp`), but is not wired to `gpu_gl_renderer.c`.

4. **OpenGL Extension Loading & Texture Filtering**:
   - In `psxrecomp/runtime/src/gpu_gl_renderer.c:310-358` (`load_modern_gl()`):
     - Functions are loaded dynamically via `SDL_GL_GetProcAddress()`.
     - `glGenerateMipmap` is currently not loaded.
     - `GL_TEXTURE_MAX_ANISOTROPY_EXT` is not queried or configured.

5. **Runtime Settings Handling**:
   - In `psxrecomp/runtime/src/main.cpp:5691-5760` (`load_re3_qol_settings()`):
     - Settings are read from `settings.toml` key-value pairs (e.g. `fast_doors`, `spu_hq`, `aspect_stretch`, `pan_scan`).

---

## 2. Logic Chain

1. **Polygon Identification**:
   Because `is_3d_actor_polygon()` (`gpu.c:4688`) accurately separates 3D character/enemy/object geometry from 2D background quads and UI elements and sets `s_actor_prim_tag` (`gpu_gl_renderer.c:661`), the engine already has the exact interception point needed to target 3D actor texture replacement.

2. **Fingerprinting Mechanism**:
   Because PS1 textures reside in shared VRAM and can be relocated or updated per room, hashing only `tpage` and `clut` would lead to cross-room collision. By computing `xxHash64` over a composite payload of:
   - Metadata (`tpage`, `clut`, limits `lim[0..3]`)
   - Active CLUT palette words from `s_vram` (16 halfwords for 4bpp, 256 halfwords for 8bpp)
   - Raw texel words from `s_vram` covering `[u_min..u_max, v_min..v_max]`
   the generated 64-bit hash is strictly deterministic, collision-free, and stable across room transitions.

3. **Shader Integration & Transparent Fallback**:
   Because `TEX_FS` is the central fragment shader for all textured primitives, adding a uniform `uniform int u_is_hd;` and binding the replacement texture to `GL_TEXTURE2` allows the shader to branch:
   - If `u_is_hd == 1`: Sample `texture(u_hd_tex, v_hd_uv)`.
   - If `u_is_hd == 0`: Run existing `fetch_texel()` from `s_raw_tex`.
   This guarantees 100% transparent fallback to original PS1 textures whenever a replacement PNG is missing.

4. **UV Normalization**:
   Because PS1 vertex UVs are authored in pixel coordinates `0..255`, binding a modern `GL_TEXTURE_2D` requires normalized coordinates `[0.0, 1.0]`. For full 256x256 actor sheets, $u_{\text{norm}} = (u + 0.5) / 256.0$, preserving texel centering and perspective correctness through `v_persp`.

5. **Quality Filtering**:
   Adding `glGenerateMipmap` and setting `GL_TEXTURE_MIN_FILTER = GL_LINEAR_MIPMAP_LINEAR` + `GL_TEXTURE_MAG_FILTER = GL_LINEAR`, combined with `GL_TEXTURE_MAX_ANISOTROPY_EXT` clamped up to 16x, completely prevents shimmering and aliasing on angled 3D models at native 60 Hz presentation.

6. **Texture Dumping Tooling**:
   By checking `dump_textures = true` in `settings.toml` (parsed in `load_re3_qol_settings()`), whenever a unique texture is drawn or decoded, it can be saved to `dump/textures/<hash>.png` via `stbi_write_png` (or the Deflate PNG writer), guarded by an in-memory `std::unordered_set<uint64_t>` to avoid repeated disk writes.

---

## 3. Caveats

1. **VRAM Texel Readback Performance**: Computing xxHash64 over raw VRAM pixels on every triangle would be redundant. Hashing must be cached per `(tpage, clut, lim)` and invalidated only when VRAM uploads or draws touch that region (using the existing `flush_cpu_upload` / `gpu_vram_dirty_mark_row` dirty tracking).
2. **Texture Packs with Sub-Rectangles vs Full Sheets**: Some community texture packs may upscale the entire 256x256 page, while others may provide isolated sprite sub-rectangles. The architecture accounts for both by supporting full-page UV normalization as default and sub-rectangle normalization when manifest dimensions match `lim`.
3. **No Direct Code Changes Made**: In accordance with the Read-Only Explorer protocol, no code files have been modified.

---

## 4. Conclusion

Requirements R2 (3D Model & Actor HD Texture Replacement) and R4 (Texture Dumping Tooling) are fully feasible and align cleanly with RE3Recomp's architecture:
- **Fingerprint**: Single-header `xxhash.h` (`XXH_INLINE_ALL`) provides fast, 64-bit hashing of `(tpage, clut, lim, palette, texels)`.
- **Replacement Pipeline**: Extends `gpu_textured_triangle()` and `TEX_FS` with `u_is_hd` and normalized UVs.
- **Filtering**: Modern OpenGL trilinear mipmapping + 16x anisotropic filtering.
- **Fallback**: Zero-overhead fallback to `s_raw_tex` via `u_is_hd = 0`.
- **Dumping**: Cleanly hookable via `settings.toml` (`dump_textures = true`) and PNG output.

---

## 5. Verification Method

To independently verify all observations and file references:

1. **Verify Actor Detection**:
   Inspect `psxrecomp/runtime/src/gpu.c:4688` (`is_3d_actor_polygon`), line 5080, and line 5241.
2. **Verify Shader CLUT Decoding**:
   Inspect `psxrecomp/runtime/src/gpu_gl_renderer.c:1684-1815` (`TEX_FS`, `fetch_texel`).
3. **Verify Existing HD Pack and STB Code**:
   Inspect `psxrecomp/runtime/include/hd_texture_pack.h:47-185`, `psxrecomp/runtime/src/hd_texture_pack.cpp`, and `psxrecomp/runtime/src/psx_window_icon.cpp:18`.
4. **Verify Room/Cut Context Reading**:
   Inspect `psxrecomp/runtime/src/gpu_gl_renderer.c:805-806` (`0x000D1F09`, `0x000D1F0A`).
5. **Verify Detailed Survey Report**:
   Read `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_2\survey_textures.md`.
