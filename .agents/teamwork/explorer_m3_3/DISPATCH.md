# Dispatch: Explorer M3-3 (3D Actor Tagging, UV Normalization & Build Integration)

## Mission
Investigate 3D actor polygon tagging in `gpu.c`, UV normalization, and CMake build integration for Milestone 3.

## Authoritative Inputs
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- Codebase paths:
  - `psxrecomp/runtime/src/gpu.c`
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`
  - `psxrecomp/CMakeLists.txt`
  - `tests/e2e/`

## Scope & Questions to Answer
1. How does `gpu.c` handle polygon rendering commands (e.g. `GPU_CMD_POLY_FT3`, `GPU_CMD_POLY_FT4`, `GPU_CMD_POLY_GT3`, `GPU_CMD_POLY_GT4`)?
2. How can 3D actor polygons be identified and tagged vs background / UI polygons? Inspect any existing tagging or state in `gpu.c` (e.g., `gl_renderer_set_actor_tag(1)`).
3. How are UV coordinates passed to the vertex shader and fragment shader in `gpu_gl_renderer.c`? When HD replacement is active (`u_is_hd == 1`), how should UVs be normalized to [0.0, 1.0] with half-texel centering `(u + 0.5) / 256.0` or vertex attribute adjustments?
4. What build system changes are needed in `CMakeLists.txt` to compile `re3_hd_textures.cpp` and link with the runtime?
5. How should `re3_hd_textures` be initialized in `main.cpp` or `re3_textures.cpp` during startup and cleanly destroyed on exit?
6. Identify potential edge cases (e.g., actor transparency/semi-transparency modes, gouraud shading, polygon subdivision, batch flushes).

Write your comprehensive findings and recommendations to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_3\handoff.md`.


## 2026-10-18T06:26:10Z
You are Explorer M3-3 (3D Actor Tagging, UV Normalization & Build Integration).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_3\
Please read:
- ORIGINAL_REQUEST: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
- PROJECT: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- DISPATCH: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_3\DISPATCH.md

Investigate 3D actor polygon identification and tagging in gpu.c, UV normalization to [0.0, 1.0] with texel centering, transparent fallback behavior, and CMakeLists.txt build integration.
Analyze gpu.c, gpu_gl_renderer.c, psxrecomp/CMakeLists.txt, and E2E test cases.
Write your complete findings and recommendations to c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_3\handoff.md.
When finished, send a message to the orchestrator notifying completion.
