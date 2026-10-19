# Handoff Report: Reviewer M3-2 (OpenGL & E2E Verification)

**Task**: Milestone 3 — 3D Model & Actor HD Texture Subsystem (OpenGL & E2E Independent Review)  
**Reviewer**: Reviewer M3-2 (`teamwork_preview_reviewer`)  
**Parent ID**: `7e5b962c-58f6-46df-a295-efcce3929cc7`  
**Verdict**: **APPROVE**  
**Governance Tier**: Level 2 (Engineering Depth / Architecture / Quality Gate Enforced)  
**Date**: 2026-10-18  

---

## 1. Observation

Direct, empirical observations recorded through independent code inspection, binary compilation, and test execution:

1. **Shader Uniforms and `TEX_FS` Additions (`gpu_gl_renderer.c`)**:
   - Lines 1706–1707:
     ```glsl
     uniform sampler2D  u_hd_tex;
     uniform int        u_is_hd;
     ```
   - Lines 1769–1775 in `TEX_FS` `main()`:
     ```glsl
     if (u_is_hd != 0) {
       vec2 hd_uv = (uv + vec2(0.5)) / 256.0;
       vec4 hd_col = texture(u_hd_tex, hd_uv);
       if (hd_col.a < 0.01) discard;
       rgb = hd_col.rgb;
       stp = (v_semi != 0 && hd_col.a < 0.99) ? 1 : (hd_col.a >= 0.5 ? 1 : 0);
     } else if (u_filter == 0) { ...
     ```
   - Line 1823:
     ```glsl
     if (v_raw == 0) rgb = clamp(rgb * v_col.rgb * 2.0, 0.0, 1.0);
     ```
   - Uniform location bindings in `make_programs()` (lines 4869–4876):
     ```c
     s_uHdTex = p_glGetUniformLocation(s_tex_prog, "u_hd_tex");
     s_uIsHd  = p_glGetUniformLocation(s_tex_prog, "u_is_hd");
     if (s_actor_prog) {
         ...
         s_actor_uHdTex = p_glGetUniformLocation(s_actor_prog, "u_hd_tex");
         s_actor_uIsHd  = p_glGetUniformLocation(s_actor_prog, "u_is_hd");
     }
     ```

2. **Texture Unit 2 (`PSXGL_TEXTURE2`) and Batch Flushing (`gpu_gl_renderer.c`)**:
   - Line 146:
     ```c
     #define PSXGL_TEXTURE2 (PSXGL_TEXTURE0 + 2)
     ```
   - Lines 3327–3335 in `flush_tex_batch()`:
     ```c
     if (s_tb_is_hd && s_tb_hd_tex != 0) {
         p_glActiveTexture(PSXGL_TEXTURE2);
         glBindTexture(GL_TEXTURE_2D, s_tb_hd_tex);
         p_glUniform1i(s_uHdTex, 2);
         p_glUniform1i(s_uIsHd, 1);
     } else {
         p_glUniform1i(s_uIsHd, 0);
     }
     p_glActiveTexture(PSXGL_TEXTURE0);
     ```
   - Lines 3775–3810 in `gpu_textured_triangle()`:
     ```c
     GLuint cur_hd_tex = 0;
     int cur_is_hd = 0;
     if (s_actor_prim_tag && g_re3_tex_config.hd_models) {
         uint16_t clut = (uint16_t)(((clut_y & 0x1FF) << 6) | ((clut_x >> 4) & 0x3F));
         uint64_t hash = re3_hd_tex_compute_from_vram(texpage, clut, lim, gpu_get_vram());
         if (hash != 0) {
             cur_hd_tex = re3_hd_tex_bind_replacement(hash);
             cur_is_hd = (cur_hd_tex != 0);
         }
     }
     int reason = -1;
     if (s_tb_n > 0) {
         if (s_tb_bank_tex != s_selected_bank_tex || s_tb_bank_live_clut != s_selected_bank_live_clut) reason = 0;
         else if (s_tb_hd_tex != cur_hd_tex || s_tb_is_hd != cur_is_hd) reason = 0;
         ...
     }
     if (reason >= 0) {
         s_batch_reason[reason]++;
         flush_tex_batch();
     }
     if (s_tb_n == 0) {
         ...
         s_tb_hd_tex = cur_hd_tex;
         s_tb_is_hd = cur_is_hd;
     }
     ```

3. **Actor Primitive Tagging Call-Graph in `gpu.c`**:
   - `is_3d_actor_polygon` (lines 4688–4704) classifies candidate polygons based on size rejection (`pw > 220 || ph > 180` and `pw < 2 && ph < 2`).
   - Every textured primitive entry point in `gpu.c` wraps drawing in `gl_renderer_set_actor_tag(1)` and resets with `gl_renderer_set_actor_tag(0)`:
     - `gp0_exec_textured_tri`: lines 5080–5094
     - `gp0_exec_textured_quad`: lines 5184–5205
     - `gp0_exec_shaded_textured_tri`: lines 5241–5276
     - `gp0_exec_shaded_textured_quad`: lines 5321–5348

4. **Anisotropic Filtering & Mipmaps (`re3_hd_textures.cpp`)**:
   - Lines 73–88 (`query_gl_caps`): queries `GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT`, clamped between 1.0f and 16.0f, and loads `glGenerateMipmap`.
   - Lines 159–174 (`load_replacement_png`):
     ```cpp
     if (p_glGenerateMipmap) {
         p_glGenerateMipmap(GL_TEXTURE_2D);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
     } else {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     }
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
     if (s_max_anisotropy > 1.0f) {
         glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, s_max_anisotropy);
     }
     ```

5. **Full Execution of Python E2E & Native Test Suites**:
   - `python tests/e2e/run_e2e_tests.py`:
     ```
     Total Tests: 155, Passed: 155, Failed: 0, Errors: 0, Skipped: 0
     Tier 1: 85/85 passed [PASS]
     Tier 2: 35/35 passed [PASS]
     Tier 3: 15/15 passed [PASS]
     Tier 4: 5/5 passed [PASS]
     Tier 5: 15/15 passed [PASS]
     Execution Time: 9.593 seconds
     ```
   - `.\build\test_re3_hd_textures_native.exe`:
     `Results: 5 executed, 5 passed, 0 failed`
   - `.\build\test_re3_hd_backgrounds_stress.exe`:
     `Stress Results: 5 executed, 5 passed, 0 failed`
   - `.\build\test_re3_textures_stress.exe`:
     `Results: 7 executed, 7 passed, 0 failed`
   - Full CMake Build (`build.ps1`):
     Success, exit code 0, generated `build/Resident_Evil_3__Nemesis_Recompiled.exe`.

6. **Integrity Violations Audit**:
   - Zero hardcoded golden hash lookup tables in runtime source code (`psxrecomp/runtime/src/`).
   - Standard canonical inlined xxHash64 (`XXH_INLINE_ALL`) algorithm used with zero external library bloat.
   - Genuine streaming VRAM hash without dynamic memory allocation (`re3_hd_tex_compute_from_vram`).
   - No mock or facade return values in production code.

---

## 2. Logic Chain

1. **UV Normalization and Texel Centering (Observation 1 & 5)**:
   - PS1 texel sampling is based on integer coordinates in `[0, 255]`. Direct uncentered normalization `uv / 256.0` places boundary coordinates at the edge of texels, causing border sampling artifacts and texture seams across triangle edges.
   - The shader formula `(uv + vec2(0.5)) / 256.0` shifts coordinate evaluation to the center of the texel `[0.5/256, 255.5/256]`, eliminating diagonal and edge bleeding.
   - This exact formula is mirrored in `re3_hd_tex_normalize_uv_full_page()` and verified by E2E test `TestFeature09UvNormalization` and native test 3 with zero deviation.

2. **Batch Splitting and Texture Unit Isolation (Observation 2 & 3)**:
   - OpenGL draw call batching merges adjacent textured primitives to reduce driver overhead. If HD actor geometry and native PS1 geometry were merged into the same batch, the shader would either sample the wrong texture or render PS1 geometry with HD normalization.
   - By keying the batch state on both `s_tb_hd_tex` and `s_tb_is_hd`, `gpu_textured_triangle()` detects any change in replacement texture or HD state (`s_tb_hd_tex != cur_hd_tex || s_tb_is_hd != cur_is_hd`) and triggers `flush_tex_batch()` immediately.
   - Furthermore, `flush_tex_batch()` binds the HD texture to texture unit 2 (`PSXGL_TEXTURE2`), preserving texture units 0 and 1 for native PS1 VRAM texels and CLUT palettes, and unconditionally resets the active texture unit back to `PSXGL_TEXTURE0`. This guarantees zero state leakage into subsequent native draws.

3. **Transparent Fallback Mechanics (Observation 2 & 4)**:
   - When a replacement texture is missing, corrupt, or `hd_models = false`, `re3_hd_tex_bind_replacement()` returns 0.
   - Consequently, `cur_is_hd` evaluates to 0, which keys the batch to PS1 fallback mode and sets uniform `u_is_hd = 0`.
   - In `TEX_FS`, `if (u_is_hd != 0)` is bypassed and the shader falls through directly to the original PS1 sampling pipeline (`u_filter == 0`, `u_filter == 2`, or bilinear).
   - Dynamic vertex color modulation `clamp(rgb * v_col.rgb * 2.0, 0.0, 1.0)` is preserved identically for both HD and fallback paths when `v_raw == 0`.

4. **Deterministic Hashing Parity (Observation 5 & 6)**:
   - The 16-byte metadata header `<HHHHHHI` (`tpage`, `clut`, `u_min`, `v_min`, `u_max`, `v_max`, `flags`), palette words (zero-padded to 16 or 256 words for 4bpp/8bpp), and raw texels match the Python test harness bit-for-bit.
   - Native unit tests confirm bit-for-bit match on canonical test cases (Jill, Nemesis, Weapon, UI Herb) using standard xxHash64 with seed 0.

---

## 3. Caveats

- **Headless Unit Test Execution**: In headless environments without an active OpenGL window context (such as command-line unit test executables running without SDL video initialization), `glGenTextures` returns 0. `load_replacement_png` safely detects this and activates the negative cache / PS1 fallback. In the live game binary (`Resident_Evil_3__Nemesis_Recompiled.exe`), the SDL OpenGL 3.3 context is fully initialized.
- **Texture Dumping Tooling Scope**: Feature 14 (Automated Texture Dumping Tooling) is assigned to Milestone 5. `re3_hd_tex_dump_if_enabled()` currently ensures directory creation and interface compliance.

---

## 4. Conclusion

The Milestone 3 implementation by Worker M3-1 is of outstanding quality, technically rigorous, fully conformant to `PROJECT.md`, `ORIGINAL_REQUEST.md`, and all User Rules. It contains zero integrity violations, zero facades, and passes all 155 E2E tests, 5 native unit tests, and all concurrency stress tests without error.

**Verdict**: **APPROVE**

---

## 5. Quality Review & Adversarial Challenge Report

### Quality Review Summary
**Verdict**: **APPROVE**

| Finding | Severity | Status | Details |
|---|:---:|:---:|---|
| None | N/A | PASS | All OpenGL pipeline additions, shader uniforms, batch flushing, and UV normalization are implemented accurately and robustly. |

#### Verified Claims
- Claim: `TEX_FS` adds `u_hd_tex` and `u_is_hd` uniforms with half-texel UV offset $\rightarrow$ Verified via `gpu_gl_renderer.c:1706-1775` $\rightarrow$ **PASS**
- Claim: Texture unit 2 binding with batch keying on `s_tb_hd_tex` $\rightarrow$ Verified via `gpu_gl_renderer.c:3327-3335, 3775-3810` $\rightarrow$ **PASS**
- Claim: Transparent PS1 fallback when texture missing or disabled $\rightarrow$ Verified via `re3_hd_textures.cpp:358-380`, `gpu_gl_renderer.c:3333`, E2E tests `TestFeature11TransparentFallback` $\rightarrow$ **PASS**
- Claim: 100% pass on E2E test suite $\rightarrow$ Verified via `python tests/e2e/run_e2e_tests.py` (155/155 passed) $\rightarrow$ **PASS**
- Claim: Zero build errors $\rightarrow$ Verified via `.\build.ps1` (Code 0, clean build) $\rightarrow$ **PASS**

### Adversarial Challenge Summary
**Overall Risk Assessment**: **LOW**

| Challenge | Attack Scenario | Blast Radius | Mitigation Implemented | Status |
|---|---|---|---|:---:|
| **1. Texture Unit Pollution** | HD texture bound on unit 2 remains bound or active unit left as Unit 2, corrupting subsequent PS1 draw calls | Native PS1 primitives sample wrong textures or CLUT | `flush_tex_batch()` explicitly restores active texture with `p_glActiveTexture(PSXGL_TEXTURE0)` at line 3335. | **DEFENDED** |
| **2. Batch State Corruption** | HD and non-HD primitives in same draw call causing texture bleeding | Glitched actor rendering, textures applied to world geometry | Batch state keys on both `s_tb_hd_tex` and `s_tb_is_hd`, forcing `flush_tex_batch()` on any transition. | **DEFENDED** |
| **3. Hardcoded Test Cheating** | Hardcoding hash return values or test fixture outcomes in source code | Integrity failure, non-functional on real game assets | Grep and code audit confirms zero hardcoded hashes in runtime source; all hashes computed via canonical xxHash64. | **DEFENDED** |
| **4. Negative Cache Stagnation** | Missing replacement permanently cached even if user installs pack mid-session | Mods added at runtime not recognized | `re3_hd_tex_shutdown()` and `re3_hd_tex_init()` cleanly clear both positive and negative caches on pack switch/reload. | **DEFENDED** |
| **5. VRAM Wrapping Bounds** | Actor texture spanning VRAM page boundary causes out-of-bounds memory access | Crash / Segmentation Fault | `re3_hd_tex_compute_from_vram` calculates `first_chunk = 1024 - x` and `second_chunk = row_words - first_chunk`, splitting updates across the 1024-word row seam safely. | **DEFENDED** |

---

## 6. Verification Method

To independently reproduce the verification:

1. **Verify Full Clean Build**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\build.ps1
   ```
   *Expected Output*: Build succeeds with code 0; produces `build/Resident_Evil_3__Nemesis_Recompiled.exe`.

2. **Verify Native C++ Unit Tests**:
   ```powershell
   .\build\test_re3_hd_textures_native.exe
   ```
   *Expected Output*: `Results: 5 executed, 5 passed, 0 failed`.

3. **Verify Full Python E2E Test Suite**:
   ```powershell
   python tests/e2e/run_e2e_tests.py
   ```
   *Expected Output*: `Total Tests: 155, Passed: 155, Failed: 0, Errors: 0, Skipped: 0` across Tiers 1–5.
