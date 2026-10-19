# Dispatch: Forensic Auditor M3-1 (Integrity Forensics & Call-Graph Verification)

## Mission
Perform comprehensive forensic integrity auditing on Milestone 3: 3D Model & Actor HD Texture Subsystem.

## Authoritative Inputs
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- Worker M3-1 Handoff: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m3_1\handoff.md`

## Audit Verification Mandate
As the Forensic Auditor, you possess binary veto authority. You must audit the entire Milestone 3 implementation for:
1. **Authenticity of Implementation**:
   - Verify that `re3_hd_textures.cpp/.h` and `xxhash.h` contain genuine logic and not mock/stub/facade wrappers.
   - Verify that xxHash64 is genuinely calculated from input payloads and not hardcoded to match fixtures.
   - Verify that `gpu_gl_renderer.c` actually binds texture unit 2 and executes the shader branch with real OpenGL calls.
2. **End-to-End Call Graph Tracing**:
   - Trace the exact path from primitive execution in `gpu.c` (`gp0_exec_*` -> `is_3d_actor_polygon` -> `gl_renderer_set_actor_tag(1)`) to `gpu_textured_triangle()` in `gpu_gl_renderer.c` -> `re3_hd_tex_bind_replacement()` -> `flush_tex_batch()`.
   - Trace lifecycle from `main.cpp` -> `re3_textures_init()` -> `re3_hd_tex_init()` and `re3_textures_shutdown()` -> `re3_hd_tex_shutdown()`.
3. **No Silent Omission / No Regression**:
   - Verify that M1 (Settings & Config) and M2 (Backgrounds & Occlusion masks) remain 100% active and unmodified by M3 changes.
   - Verify that vanilla PS1 VRAM sampling continues to function whenever `hd_models = false` or replacements are missing.
4. **Binary Audit Verdict**:
   - State your verdict clearly as **CLEAN** or **INTEGRITY VIOLATION** in `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\auditor_m3_1\handoff.md`.

## 2026-10-18T06:57:06Z
You are Forensic Auditor M3-1 (teamwork_preview_auditor).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\auditor_m3_1\

Please read:
- ORIGINAL_REQUEST: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
- PROJECT: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- DISPATCH: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\auditor_m3_1\DISPATCH.md
- Worker M3-1 Handoff: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m3_1\handoff.md

Perform forensic integrity checks: verify authenticity of re3_hd_textures.cpp/.h, xxhash.h, gpu_gl_renderer.c changes, end-to-end call graph from gpu.c primitive execution to OpenGL drawing, verify no stubs/facades/hardcoded test fixtures, and verify zero regressions on M1 and M2.
Write your complete report to c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\auditor_m3_1\handoff.md with a clear binary verdict: CLEAN or INTEGRITY VIOLATION.
Notify the orchestrator via send_message when done.
