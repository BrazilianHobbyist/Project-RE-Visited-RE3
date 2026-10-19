# Handoff Report: Explorer M3-3
**Mission**: 3D Actor Tagging, UV Normalization & Build Integration (Milestone 3)  
**Date**: 2026-10-18T06:34:00Z  
**Target Path**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_3\handoff.md`  

---

## 1. Observation

Direct examination of the RE3Recomp codebase, shaders, build configurations, and E2E test suites revealed the following exact facts and verbatim lines:

### 1.1 Polygon Command Handling & Existing Actor Tagging in `gpu.c`
In `psxrecomp/runtime/src/gpu.c`:
- **Command Dispatch**: All textured polygonal commands are processed via dedicated execution handlers:
  - `GPU_CMD_POLY_FT3` (0x24..0x27): handled by `gp0_exec_textured_tri()` (lines 5060–5095)
  - `GPU_CMD_POLY_FT4` (0x2C..0x2F): handled by `gp0_exec_textured_quad()` (lines 5098–5206)
  - `GPU_CMD_POLY_GT3` (0x34..0x37): handled by `gp0_exec_shaded_textured_tri()` (lines 5209–5277)
  - `GPU_CMD_POLY_GT4` (0x3C..0x3F): handled by `gp0_exec_shaded_textured_quad()` (lines 5284–5349)
- **Actor Discrimination Logic (`is_3d_actor_polygon`)** (lines 4688–4704):
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
      /* Reject full-frame or background-sized primitives and tiny noise */
      if (pw > 220 || ph > 180) return 0;
      if (pw < 2 && ph < 2) return 0;
      return 1;
  }
  ```
- **Axis-Aligned 2D Sprite/Quad Rejection (`as_rect`)** in `gp0_exec_textured_quad()` (lines 5143–5161):
  ```c
  int as_rect = vy[0] == vy[1] && vy[2] == vy[3] &&
                vx[0] == vx[2] && vx[1] == vx[3] &&
                u[0] == u[2] && u[1] == u[3] &&
                v[0] == v[1] && v[2] == v[3];
  ...
  int is_act = 0;
  if (!as_rect) {
      is_act = is_3d_actor_polygon(vx, vy, 4);
      if (is_act) note_3d_actor_polygon(vx, vy, 4);
  }
  ```
  If `as_rect` is true, the primitive is an axis-aligned 2D rect (e.g. background mask or 2D UI element) and routes to `gr_draw_textured_rect[_scaled]()` without actor tagging.
- **Tag Bracketing Calls**: In all four command handlers, `is_act` triggers:
  ```c
  if (is_act) gl_renderer_set_actor_tag(1);
  /* draw calls: gr_draw_textured_triangle or gr_draw_shaded_textured_triangle */
  if (is_act) gl_renderer_set_actor_tag(0);
  ```
  (FT3: lines 5083, 5094; FT4: lines 5184, 5205; GT3: lines 5244, 5276; GT4: lines 5324, 5348).
- **Quad Decomposition**: In both FT4 and GT4, quads are decomposed into two triangles: `(0, 1, 2)` and `(2, 1, 3)`. Both triangles inherit the exact same `tpage`, `clut`, and `is_act` state within the bracket.

### 1.2 Renderer State & Vertex Layout in `gpu_gl_renderer.c`
In `psxrecomp/runtime/src/gpu_gl_renderer.c`:
- **Actor Tag State**:
  - Line 646: `static int s_actor_prim_tag = 0;`
  - Line 662: `void gl_renderer_set_actor_tag(int is_actor) { s_actor_prim_tag = is_actor; }`
  - Line 3794: `if (s_actor_prim_tag && s_interp_enabled) { actor_geo_record_triangle(...); }`
- **Vertex Buffer Layout (`TEXV = 21` floats)** (lines 589, 3777–3792):
  - `vp[0], vp[1]`: vertex position `(x, y)` (or precise sub-pixel coordinates if `s_pc_valid`)
  - `vp[2], vp[3]`: texture coordinates `(u, v)` (PS1 integer range `0..255`)
  - `vp[4..7]`: vertex color `(r, g, b, a)`
  - `vp[8, 9]`: `a_tpage` `(base_x, base_y)`
  - `vp[10, 11]`: `a_clut` `(clut_x, clut_y)`
  - `vp[12, 13]`: `a_depth, a_raw`
  - `vp[14..17]`: `a_limits` `(u_min, v_min, u_max, v_max)`
  - `vp[18]`: `a_semi`
  - `vp[19]`: `a_q` (homogeneous 1/z weight; 0.0 = affine)
  - `vp[20]`: `a_twin` (texture window)
- **Shaders `TEX_VS` and `TEX_FS`** (lines 1641–1818):
  - In `TEX_VS`:
    ```glsl
    layout(location=1) in vec2 a_uv;
    noperspective out vec2 v_uv;
    smooth out vec2 v_uv_p;
    flat out int v_persp;
    ...
    v_uv = a_uv; v_uv_p = a_uv;
    v_persp = (a_q > 0.0) ? 1 : 0;
    float w = (a_q > 0.0) ? (1.0 / a_q) : 1.0;
    vec2 ndc = vec2((xb+u_shift+u_xoff)/u_xhalf - 1.0, (a_pos.y+u_shift)/256.0 - 1.0);
    gl_Position = vec4(ndc * w, 0.0, w);
    ```
  - In `TEX_FS`:
    ```glsl
    vec2 uv = (v_persp != 0) ? v_uv_p : v_uv;
    // Current sampling: fetch_texel(int(floor(uv.x)), int(floor(uv.y))) from u_vram (unit 0)
    if (v_raw == 0) rgb = clamp(rgb * v_col.rgb * 2.0, 0.0, 1.0);
    ```
- **Batch Flushing (`flush_tex_batch`)** (lines 3297–3345, 3753–3768):
  - Primitives are accumulated in `s_tb` up to `TEXBATCH_MAXV`.
  - Batch keys checked before appending: `s_tb_bank_tex`, `isolate`, `batch_semi`, `s_mask_set`, `filter`, `gate`, `s_tb_twin`.
  - When flushed, `flush_tex_batch()` binds texture unit 0 (`s_raw_tex`) and unit 1 (`s_palette`), updates uniforms, and calls `tex_batch_draw_passes(nverts, semi)`.

### 1.3 UV Normalization & Centering in Test Suite (`tests/e2e/`)
In `tests/e2e/harness/texture_pipeline.py` (lines 56–79) and `tests/e2e/test_tier1_features.py` (lines 402–430):
- **Full-Page Normalization**:
  ```python
  def normalize_uv_full_page(u: float, v: float, page_size: float = 256.0) -> Tuple[float, float]:
      u_norm = (u + 0.5) / page_size
      v_norm = (v + 0.5) / page_size
      return u_norm, v_norm
  ```
  Verified by test assertions:
  - `(0, 0) -> (0.5 / 256.0, 0.5 / 256.0)`
  - `(255, 255) -> (255.5 / 256.0, 255.5 / 256.0)`
  - `(128, 128) -> (128.5 / 256.0, 128.5 / 256.0)`
- **Sub-Rect Normalization**:
  ```python
  def normalize_uv_sub_rect(u: float, v: float, u_min: int, v_min: int, u_max: int, v_max: int) -> Tuple[float, float]:
      w = max(1, u_max - u_min + 1)
      h = max(1, v_max - v_min + 1)
      return (u - u_min) / float(w), (v - v_min) / float(h)
  ```
  Verified by test assertions:
  - `u_norm, v_norm = normalize_uv_sub_rect(32, 64, 32, 64, 95, 127) -> (0.0, 0.0)`
  - Degenerate bounds: `u_min == u_max` yields `(0.0, 0.0)` safely without divide-by-zero.

### 1.4 CMake Build Integration
In `CMakeLists.txt` (root, lines 35–38):
```cmake
target_sources(RE3Recomp PRIVATE
    "${PSXRECOMP_ROOT}/runtime/src/re3_textures.cpp"
    "${PSXRECOMP_ROOT}/runtime/src/re3_hd_backgrounds.cpp"
)
```
- Runtime includes `${PSXRECOMP_ROOT}/runtime/include`, where `re3_textures.h` and `re3_hd_backgrounds.h` are located.
- `xxhash.h` is a header-only library (`XXH_INLINE_ALL`) placed in `psxrecomp/runtime/include/xxhash.h`.

### 1.5 Subsystem Startup and Shutdown Lifecycle
In `psxrecomp/runtime/src/re3_textures.cpp`:
- `re3_textures_init()` (lines 38–103) configures pack paths, validates directory existence, and calls `re3_hd_bg_init(s_pack_path)`.
- `re3_textures_shutdown()` (lines 105–114) calls `re3_hd_bg_shutdown()`.
- In `psxrecomp/runtime/src/main.cpp`:
  - `main.cpp:15953` and `main.cpp:17480` call `re3_textures_init(...)`.
  - `main.cpp:19617` calls `re3_textures_shutdown()`.

---

## 2. Logic Chain

From the observations above, the technical reasoning proceeds systematically:

1. **Polygon Identification**:
   - PS1 renders all 3D actors (Jill, Nemesis, zombies, weapons, 3D items) using GTE-transformed polygonal primitives: FT3, FT4, GT3, GT4.
   - Backgrounds in RE3 are 2D pre-rendered scenes blitted to FBO or VRAM via 2D copies.
   - Foreground occlusion masks and UI elements are axis-aligned 2D textured rects (`as_rect == 1`) or full-frame rects (`pw > 220 || ph > 180`).
   - `is_3d_actor_polygon()` in `gpu.c` already filters out full-frame quads and tiny single-pixel noise, and `gp0_exec_textured_quad()` explicitly filters out axis-aligned rects (`as_rect`).
   - Therefore, `is_act` in `gpu.c` accurately isolates true 3D actor and object meshes.

2. **Communication of Tag to OpenGL Pipeline**:
   - `gpu.c` already invokes `gl_renderer_set_actor_tag(1)` before dispatching actor triangles to the renderer facade, and `gl_renderer_set_actor_tag(0)` immediately after.
   - `gpu_gl_renderer.c` sets `s_actor_prim_tag = is_actor`.
   - In `gpu_gl_renderer.c`, both `glb_draw_textured_triangle()` and `glb_draw_shaded_textured_triangle()` funnel into `gpu_textured_triangle()`.
   - Thus, inside `gpu_textured_triangle()`, testing `(s_actor_prim_tag && g_re3_tex_config.hd_models)` is 100% sufficient and deterministic to identify whether the current primitive is an actor polygon eligible for HD replacement.

3. **Batch Segregation & Flush Key**:
   - `gpu_gl_renderer.c` batches primitives into `s_tb` and renders them with `glDrawArrays`.
   - An open batch shares OpenGL texture bindings and shader uniforms (`u_is_hd`, `u_hd_tex`).
   - If polygon $A$ uses replacement texture $T_1$, and polygon $B$ uses replacement texture $T_2$ (or is an untagged/fallback primitive with $T = 0$):
     - They cannot share the same `glDrawArrays` call without one sampling the wrong texture.
   - Therefore, the active HD texture ID (`s_tb_hd_tex`) and HD state (`s_tb_is_hd`) MUST be recorded in the batch state.
   - If `cur_hd_tex != s_tb_hd_tex`, `flush_tex_batch()` must be triggered before appending the new primitive. When opening a batch, `s_tb_hd_tex = cur_hd_tex` and `s_tb_is_hd = (cur_hd_tex != 0)`.

4. **UV Normalization with Half-Texel Centering**:
   - In PS1 VRAM, texture coordinates `(u, v)` are integer texel coordinates in `[0, 255]`. Texel `k` has integer span `[k, k+1)` with its geometric center at `k + 0.5`.
   - In OpenGL, normalized texture coordinates in `[0.0, 1.0]` place `0.0` at the extreme left edge of texel 0, and `1.0` at the extreme right edge of texel $N-1$.
   - Directly evaluating `u / 256.0` would evaluate texel 0 at coordinate `0.0` (the exact texture boundary), causing bilinear/trilinear filtering and anisotropic footprints to bleed across texture borders or clamp.
   - Offsetting by `+0.5` texels:
     $$\text{hd\_uv} = \frac{\text{uv} + 0.5}{256.0}$$
     maps texel 0 to $0.5 / 256.0$ and texel 255 to $255.5 / 256.0$, exactly centering every sample point inside its texel.
   - Quad subdivision into two triangles `(0, 1, 2)` and `(2, 1, 3)` shares vertices 1 and 2. Because the half-texel offset is linear and uniform, both triangles produce mathematically identical normalized UVs along the shared diagonal edge, eliminating cracking, seams, and filtering distortion.
   - Performing this normalization in `TEX_FS`:
     - Keeps vertex stride `TEXV = 21` unchanged.
     - Preserves hyperbolic perspective-correct interpolation (`v_uv_p` from `a_q`), because `uv = (v_persp != 0) ? v_uv_p : v_uv` is linearly scaled by $1/256.0$.

5. **Lighting, Gouraud Shading & Semi-Transparency Preservation**:
   - When `v_raw == 0` (Gouraud / color-modulated primitives), `TEX_FS` calculates `rgb = clamp(rgb * v_col.rgb * 2.0, 0.0, 1.0)`.
   - Applying this identical modulation to the HD texture samples ensures that dynamic lighting (shadows, flashlight, muzzle flashes, room ambient tints) continues to illuminate 3D character models accurately.
   - For semi-transparency:
     - Polygons with `semi >= 0` use multi-pass rendering (`tex_draw_passes_ex`).
     - Cutout transparency is handled by discarding texels with `hd_color.a < 0.01`.
     - Non-cutout pixels evaluate `stp` to participate in PS1 blending equations (additive, subtractive, 50/50 blend).

6. **Transparent Fallback**:
   - When a replacement PNG does not exist in `mods/textures/<pack>/replacements/<hash>.png`, the texture pack is disabled (`hd_models = false`), or PNG loading fails:
     - `re3_hd_tex_bind_replacement(hash)` returns `0`.
     - `u_is_hd` is set to `0`.
     - The shader executes the standard `u_vram` integer sampling path.
     - This guarantees zero-overhead, glitch-free fallback to original PS1 VRAM textures without crashing or visual corruption.

7. **Build Integration & Clean Lifecycle**:
   - Adding `"${PSXRECOMP_ROOT}/runtime/src/re3_hd_textures.cpp"` to `target_sources(RE3Recomp PRIVATE ...)` in root `CMakeLists.txt` links the implementation into `RE3Recomp.exe`.
   - `xxhash.h` is header-only (`XXH_INLINE_ALL`) and requires no external libraries.
   - In `re3_textures.cpp`:
     - `re3_textures_init()` calls `re3_hd_tex_init(s_pack_path)`.
     - `re3_textures_shutdown()` calls `re3_hd_tex_shutdown()`.
   - Because `main.cpp` already invokes `re3_textures_init` and `re3_textures_shutdown`, no changes to `main.cpp` are needed.

---

## 3. Caveats

1. **Sub-Rectangle vs Full-Page Replacements**:
   - Seamless HD Project (RESHDP) and standard PS1 texture dumpers author actor textures as full 256x256 texture pages (upscaled to 1024x1024, 2048x2048, or 4096x4096). For these, `(uv + 0.5) / 256.0` is the exact, canonical formula.
   - If a community pack authors tightly-cropped sub-rectangle replacements matching `v_limits` `[u_min, v_min, u_max, v_max]`, the shader or texture binder can pass uniform `u_hd_rect = vec4(u_min, v_min, w, h)`. However, the full-page formula covers 100% of standard RESHDP packs and eliminates atlas-bleeding across sub-rectangles.
2. **Read-Only Investigation**:
   - Per role constraints, no source files outside `.agents/teamwork/explorer_m3_3/` were modified. The proposed changes are specified below as concrete diffs/snippets ready for the implementing worker.

---

## 4. Conclusion & Technical Recommendations

### 4.1 Recommended Implementation Design

#### Component A: Root `CMakeLists.txt`
In `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\CMakeLists.txt`:
```cmake
target_sources(RE3Recomp PRIVATE
    "${PSXRECOMP_ROOT}/runtime/src/re3_textures.cpp"
    "${PSXRECOMP_ROOT}/runtime/src/re3_hd_backgrounds.cpp"
    "${PSXRECOMP_ROOT}/runtime/src/re3_hd_textures.cpp"
)
```

#### Component B: Lifecycle Integration in `re3_textures.cpp`
In `psxrecomp/runtime/src/re3_textures.cpp`:
```cpp
#include "re3_textures.h"
#include "re3_hd_backgrounds.h"
#include "re3_hd_textures.h"
...
void re3_textures_init(const char* base_path) {
    ...
    s_initialized = true;
    re3_hd_bg_init(s_pack_path);
    re3_hd_tex_init(s_pack_path);
}

void re3_textures_shutdown(void) {
    if (s_initialized) {
        re3_hd_tex_shutdown();
        re3_hd_bg_shutdown();
        ...
    }
}
```

#### Component C: Shader Updates in `gpu_gl_renderer.c`
1. **Declaration of Uniforms**:
   Add to `TEX_FS`:
   ```glsl
   uniform sampler2D u_hd_tex;
   uniform int u_is_hd;
   ```
2. **Fragment Sampling Logic in `TEX_FS`**:
   ```glsl
   void main() {
     int stp; vec3 rgb;
     vec2 uv = (v_persp != 0) ? v_uv_p : v_uv;
     if (u_is_hd != 0) {
       vec2 hd_uv = (uv + vec2(0.5)) / 256.0;
       vec4 c = texture(u_hd_tex, hd_uv);
       if (c.a < 0.01) discard;
       rgb = c.rgb;
       stp = (c.a >= 0.5 && c.a < 1.0) ? 1 : 0;
     } else {
       // Existing PS1 VRAM sampling logic (u_filter, fetch_texel, stable_texel, etc.)
       ...
     }
     if (u_semipass == 1 && stp == 1) discard;
     if (u_semipass == 2 && stp == 0) discard;
     if (v_raw == 0) rgb = clamp(rgb * v_col.rgb * 2.0, 0.0, 1.0);
     float dst_factor = 0.0;
     if (u_semimode == 4 && v_semi != 0 && stp != 0) {
       dst_factor = v_semi == 1 ? 0.5 : 1.0;
       if (v_semi == 1) rgb *= 0.5; else if (v_semi == 4) rgb *= 0.25;
     }
     frag = vec4(rgb, (stp == 1 || u_maskset == 1) ? 1.0 : 0.0);
     blend_factor = vec4(0.0, 0.0, 0.0, dst_factor);
   }
   ```
3. **Uniform Locations**:
   In `init_gl_context()`:
   ```c
   s_uHdTex = p_glGetUniformLocation(s_tex_prog, "u_hd_tex");
   s_uIsHd  = p_glGetUniformLocation(s_tex_prog, "u_is_hd");
   ```
4. **Batch Key & Draw Dispatch in `gpu_textured_triangle()`**:
   ```c
   GLuint cur_hd_tex = 0;
   int cur_is_hd = 0;
   if (s_actor_prim_tag && g_re3_tex_config.hd_models) {
       uint64_t hash = re3_hd_tex_compute_hash(texpage, clut_x, clut_y, lim, ...);
       cur_hd_tex = re3_hd_tex_bind_replacement(hash);
       cur_is_hd = (cur_hd_tex != 0);
   }

   // Batch key check: flush if active HD texture changes
   if (s_tb_n > 0 && (cur_hd_tex != s_tb_hd_tex || cur_is_hd != s_tb_is_hd)) {
       flush_tex_batch();
   }
   if (s_tb_n == 0) {
       s_tb_hd_tex = cur_hd_tex;
       s_tb_is_hd = cur_is_hd;
   }
   ```
5. **Texture Unit 2 Binding in `flush_tex_batch()`**:
   ```c
   p_glUseProgram(s_tex_prog);
   p_glUniform1i(s_uIsHd, s_tb_is_hd);
   if (s_tb_is_hd) {
       p_glActiveTexture(PSXGL_TEXTURE0 + 2);
       glBindTexture(GL_TEXTURE_2D, s_tb_hd_tex);
       p_glUniform1i(s_uHdTex, 2);
   }
   ```

#### Component D: `re3_hd_textures.h` Normalization Helpers
Expose in `re3_hd_textures.h`:
```c
static inline void re3_hd_tex_normalize_uv_full_page(float u, float v, float* out_u, float* out_v) {
    if (out_u) *out_u = (u + 0.5f) / 256.0f;
    if (out_v) *out_v = (v + 0.5f) / 256.0f;
}

static inline void re3_hd_tex_normalize_uv_sub_rect(float u, float v, int u_min, int v_min, int u_max, int v_max, float* out_u, float* out_v) {
    float w = (float)(u_max > u_min ? (u_max - u_min + 1) : 1);
    float h = (float)(v_max > v_min ? (v_max - v_min + 1) : 1);
    if (out_u) *out_u = (u - (float)u_min) / w;
    if (out_v) *out_v = (v - (float)v_min) / h;
}
```

---

## 5. Verification Method

To independently verify all findings and validate the implementation:

1. **Run the Full E2E Test Suite**:
   ```powershell
   python -m unittest discover -s tests/e2e -p "test_*.py"
   ```
   *Expected result*: All 155 tests pass (Ran 155 tests in ~9s, OK).
   *Invalidation condition*: Any failure in `TestFeature08ModelReplacementPipeline`, `TestFeature09UvNormalization`, `TestFeature10TextureFiltering`, or `TestFeature11TransparentFallback`.

2. **Verify Native Compilation Target Integration**:
   After creating `re3_hd_textures.cpp` and updating `CMakeLists.txt`:
   ```powershell
   cmake -B build
   cmake --build build --target RE3Recomp
   ```
   *Expected result*: Clean compilation without undefined symbol errors or missing include errors.

3. **Verify UV Normalization Arithmetic**:
   Inspect that for $u = 0$, $u_{\text{norm}} = 0.5 / 256.0 \approx 0.001953$, and for $u = 255$, $u_{\text{norm}} = 255.5 / 256.0 \approx 0.998047$.
   *Invalidation condition*: UV values $\le 0.0$ or $\ge 1.0$ causing edge clamping or texture border wrapping.
