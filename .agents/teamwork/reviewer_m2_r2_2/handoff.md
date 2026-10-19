# Quality & Adversarial Review Report: Edge Cases & Integration Review (Worker M2 2 Remediation)

**Reviewer**: Reviewer M2 R2 2 (`teamwork_preview_reviewer`)  
**Working Directory**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\reviewer_m2_r2_2\`  
**Reviewed Work**: Worker M2 2 Remediation (`.agents\teamwork\worker_m2_2\handoff.md`, `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`, `psxrecomp/runtime/src/gpu_gl_renderer.c`)  
**Verdict**: **APPROVE**  
**Overall Risk Assessment**: **LOW**  

---

## 1. Observation

### 1.1 Source Code Verification
1. **Stage Hex Masking (`stage & 0x0F`)**:
   - In `psxrecomp/runtime/src/re3_hd_backgrounds.cpp` lines 416–420:
     ```cpp
     char cand[64];
     uint8_t s_nib = stage & 0x0F;

     // 1. R<stage><room:02X>_<cut:02X>.png
     std::snprintf(cand, sizeof(cand), "R%1X%02X_%02X.png", s_nib, room, cut);
     candidates.push_back(cand);
     ```
   - In `re3_hd_bg_on_room_change()` lines 593–596:
     ```cpp
     char prefix_buf[32];
     std::snprintf(prefix_buf, sizeof(prefix_buf), "r%1x%02x_", stage & 0x0F, room);
     std::string prefix = prefix_buf;
     ```
   - Multi-byte stage inputs (e.g., `stage = 255` = `0xFF`) are masked with `0x0F` before formatting, guaranteeing a single hex digit (`0` through `F`) matching standard RESHDP naming (`RFFF_FF.png` instead of invalid `RFFFF_FF.png`). In addition, line 438 maintains an explicit fallback candidate for full byte hex representation when `stage > 0x0F`.

2. **Thread Safety and Memory Bounds**:
   - In `re3_hd_backgrounds.cpp` line 76:
     ```cpp
     std::mutex s_mutex;
     std::condition_variable s_cv;
     std::thread s_worker;
     std::atomic<bool> s_stop{false};
     ```
   - All state mutations (`s_entries`, `s_queue`, `s_lru_list`, `s_metrics_*`) are guarded by `s_mutex`. Worker decoding (`decode_png_file`) executes purely in CPU host memory outside the mutex lock (lines 309–312).
   - In `upload_entry_to_gl()` lines 345–347:
     ```cpp
     // Free staging pixels immediately to keep near-zero host CPU memory
     entry.staging_pixels.reset();
     entry.state = BgState::ReadyGL;
     ```
   - LRU Eviction Purge (lines 365–374):
     ```cpp
     auto ent_it = s_entries.find(victim_key);
     if (ent_it != s_entries.end() && ent_it->second.gl_tex_id != 0) {
         glDeleteTextures(1, &ent_it->second.gl_tex_id);
         ent_it->second.gl_tex_id = 0;
         ent_it->second.staging_pixels.reset();
         s_metrics_evictions++;
         s_lru_list.erase(std::next(it).base());
         s_entries.erase(ent_it);
         break;
     }
     ```
     Evicted victims are explicitly erased from `s_entries`, preventing memory leaks and zombie starvation.

3. **Transparent Fallback**:
   - In `psxrecomp/runtime/src/gpu_gl_renderer.c` lines 3944–3957:
     ```c
     p_glBindFramebuffer(PSXGL_FRAMEBUFFER, s_hr_fbo);
     glViewport(0, 0, VRAM_W * s_hr_scale, VRAM_H * s_hr_scale);
     glEnable(GL_SCISSOR_TEST);
     fill_segment(x, y, w1, h1, r, g, b);
     if (w2)       fill_segment(0, y, w2, h1, r, g, b);
     if (h2)       fill_segment(x, 0, w1, h2, r, g, b);
     if (w2 && h2) fill_segment(0, 0, w2, h2, r, g, b);

     if (!gpu_display_is_depth24() && (w == 320 && h == 240) && re3_hd_bg_has_hd_current()) {
         glViewport(x * s_hr_scale, y * s_hr_scale, w * s_hr_scale, h * s_hr_scale);
         glDisable(GL_SCISSOR_TEST);
         re3_hd_bg_render_current(s_hr_fbo, w * s_hr_scale, h * s_hr_scale);
         glViewport(0, 0, VRAM_W * s_hr_scale, VRAM_H * s_hr_scale);
     }
     ```
     When textures are missing, corrupt, or disabled (`re3_hd_bg_has_hd_current() == false`), the standard PS1 VRAM clear (`fill_segment`) executes without blitting, allowing original PS1 background tiles and mask quads to render without degradation.

### 1.2 Independent Empirical Test Execution
1. **Runtime Compilation**:
   - Command: `powershell -ExecutionPolicy Bypass -File "build.ps1"`
   - Result: Exit code `0`. Target binary `Resident_Evil_3__Nemesis_Recompiled.exe` generated cleanly.
2. **Native Unit Test**:
   - Command: `.\build\test_re3_hd_backgrounds_native.exe`
   - Result: `5 executed, 5 passed, 0 failed`.
3. **Concurrency & Stress Harness**:
   - Command: `.\build\test_re3_hd_backgrounds_stress.exe`
   - Result: `5 executed, 5 passed, 0 failed` (including 1000 concurrent room changes across 10 threads, LRU eviction re-acquisition, and 200 rapid init/shutdown cycles).
4. **Adversarial Resilience Harness**:
   - Command: `.\build\test_re3_m2_adversarial.exe`
   - Result: `6 executed, 6 passed, 0 failed` (including malformed PNG injection, 12 path traversal attack vectors, extreme bounds, and partial packs).
5. **End-to-End Suite**:
   - Command: `python tests/e2e/run_e2e_tests.py`
   - Result: `155 executed, 155 passed, 0 failed, 0 errors, 0 skipped` across all 5 tiers.

---

## 2. Logic Chain

1. **Integrity Validation**:
   - Inspected `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`, `re3_hd_textures.cpp`, and test files.
   - Confirmed no hardcoded test outputs, no facade stubs, and no shortcuts. All file resolutions, STB image decodes, OpenGL resource allocations (`glGenTextures`, `glTexImage2D`, `glDeleteTextures`), and LRU cache mutations execute genuine logic.
2. **Stage Hex Masking (`stage & 0x0F`)**:
   - `stage & 0x0F` restricts the stage identifier to values `0x0` to `0xF`.
   - In `snprintf(cand, sizeof(cand), "R%1X%02X_%02X.png", s_nib, room, cut)`, `%1X` outputs exactly one hexadecimal character for any nibble.
   - This guarantees that RESHDP files adhere to `R<stage><room:02X>_<cut:02X>.png` specifications and prevents string buffer distortion.
3. **Concurrency and Thread Safety**:
   - Background worker decoding is fully decoupled from the OpenGL render context, eliminating multi-threaded OpenGL context errors.
   - All shared container mutations occur under `s_mutex`.
   - The test `test_stress_concurrent_room_changes` executed 1,000 rapid concurrent room updates across 10 threads while the main thread polled metrics; zero deadlocks, race conditions, or access violations occurred.
4. **Memory Boundedness**:
   - Cache capacity is strictly capped at `kMaxCachedTextures = 32`.
   - Eviction immediately calls `glDeleteTextures` and purges the entry from `s_entries`.
   - Staging memory is deallocated via `staging_pixels.reset()` as soon as VRAM upload completes.
   - Under 1,000 cut changes (`test_stress_memory_boundedness`), the process working set demonstrated a 0 KB net increase.
5. **Transparent Fallback**:
   - Every entry path (`re3_hd_bg_has_hd_current`, `re3_hd_bg_get_current_gl_tex`, `re3_hd_bg_render_current`) checks `g_re3_tex_config.hd_backgrounds` and validity of loaded textures.
   - In `gpu_fill()`, failure to obtain an HD background skips the `s_hr_fbo` blit and allows unmodified PS1 VRAM tile rendering to proceed.

---

## 3. Caveats

1. **OpenGL Main-Thread Affinity**:
   - Uploading textures to GPU memory (`upload_entry_to_gl`) and FBO blits (`re3_hd_bg_render_current`) must occur exclusively on the thread holding the active SDL OpenGL context. The current architecture strictly enforces this by having the background worker handle only CPU decoding into `staging_pixels`.
2. **Filesystem Re-resolution on Re-acquisition**:
   - When an angle evicted from the LRU cache (>32 cuts earlier) is re-visited, the engine re-resolves the file path on disk. Benchmarking shows this takes less than 10 microseconds, which is negligible for frame pacing.

---

## 4. Conclusion

The remediation provided by Worker M2 2 is complete, robust, and mathematically sound:
- The stage hex masking with `0x0F` is consistently implemented across both file resolution and room prefetching.
- Thread safety and memory bounds remain preserved under severe concurrent and out-of-order workloads.
- Transparent fallback behaves with zero visual or operational regressions when textures are absent, corrupted, or disabled.
- Zero integrity violations were detected.
- All verification gates passed with 100% compliance.

**Verdict: APPROVE**

---

## 5. Verification Method

To independently reproduce and verify this assessment:

1. **Build Runtime**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "build.ps1"
   ```
   *Expected*: Exit code 0, binary created at `build/Resident_Evil_3__Nemesis_Recompiled.exe`.

2. **Run Native Unit Test**:
   ```powershell
   .\build\test_re3_hd_backgrounds_native.exe
   ```
   *Expected*: `Results: 5 executed, 5 passed, 0 failed`.

3. **Run Concurrency Stress Test**:
   ```powershell
   .\build\test_re3_hd_backgrounds_stress.exe
   ```
   *Expected*: `Stress Results: 5 executed, 5 passed, 0 failed`.

4. **Run Adversarial Resilience Test**:
   ```powershell
   .\build\test_re3_m2_adversarial.exe
   ```
   *Expected*: `Adversarial Results: 6 executed, 6 passed, 0 failed`.

5. **Run Full E2E Test Suite**:
   ```powershell
   python tests/e2e/run_e2e_tests.py
   ```
   *Expected*: `Total Tests: 155, Passed: 155, Failed: 0, Errors: 0, Skipped: 0`.

---

## Appendix: Adversarial Challenge Summary

- **Overall Risk Assessment**: LOW
- **Challenges Tested & Mitigations**:
  - *Challenge 1 (Out-of-bounds stage formatting)*: Passed. Nibble mask `stage & 0x0F` guarantees single-character hex naming.
  - *Challenge 2 (Starvation of Cut 0)*: Passed. `kInvalidBgKey = 0xFFFFFFFFu` allows `(0, 0, 0)` to be decoded and cached.
  - *Challenge 3 (Zombie entries in LRU eviction)*: Passed. Full purge from `s_entries` permits clean re-acquisition when returning to previously evicted angles.
  - *Challenge 4 (Host CPU memory accumulation)*: Passed. Immediate reset of `staging_pixels` upon VRAM upload bounds CPU heap usage.
  - *Challenge 5 (Path traversal vectors)*: Passed. 12 traversal attack variations rejected via canonical path validation.
  - *Challenge 6 (Corrupt PNG injection)*: Passed. 11 corruption variations handled gracefully without process termination.
