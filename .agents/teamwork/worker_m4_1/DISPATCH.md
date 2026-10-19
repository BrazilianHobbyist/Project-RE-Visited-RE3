# DISPATCH LOG

## 2026-10-18T10:18:57Z
You are Worker M4-1 (teamwork_preview_worker), the HD UI & Menus Engine Implementer.
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m4_1\
Project root: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

MANDATORY FILES TO READ BEFORE STARTING:
1. c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
2. c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
3. c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_1\handoff.md

Engineering Governance:
Follow Level 2 engineering governance:
1. Pre-Flight baseline check: verify git status and clean baseline before editing.
2. Implement Milestone 4 (Features 12 & 13) per specification in explorer_m4_1/handoff.md:
   - Create `psxrecomp/runtime/include/re3_hd_ui.h` and implement `psxrecomp/runtime/src/re3_hd_ui.cpp` (and `re3_hd_ui.h` internal header if needed).
   - Implement complete pure C contract: `re3_hd_ui_init`, `re3_hd_ui_shutdown`, `re3_hd_ui_is_active`, `re3_hd_ui_resolve_backdrop_path`, `re3_hd_ui_resolve_status_backdrop` (mapping 0x04->fine, 0x03->caution, 0x02->danger, 0x01->poison, unknown->fine), `re3_hd_ui_load_backdrop`, `re3_hd_ui_has_backdrop`, `re3_hd_ui_get_backdrop_gl_tex`, `re3_hd_ui_render_backdrop`, `re3_hd_ui_is_fullscreen_quad`, `re3_hd_ui_detect_screen_from_memory`, `re3_hd_ui_intercept_menu_quad`, `re3_hd_ui_bind_sprite`, `re3_hd_ui_get_metrics`.
   - Implement texture decoding and GL texture creation with stb_image, proper filtering (bilinear/trilinear, CLAMP_TO_EDGE), and LRU / cache management.
   - Screen-spanning 2D quad geometry detection and interception in menu contexts, blitting HD backdrop to s_hr_fbo while preserving interactive overlays (fonts, cursors, item slots, 3D item inspection) on top.
   - Integrate with `psxrecomp/runtime/src/re3_textures.cpp` lifecycle, `gpu.c`, and `gpu_gl_renderer.c`.
   - Update CMakeLists.txt so `re3_hd_ui.cpp` is compiled into the runtime target.
3. Call-graph evidence: prove end-to-end invocation from initialization, runtime hooks, to rendering.
4. Verify by building the project (`powershell -ExecutionPolicy Bypass -File ./build.ps1` or cmake build) and running E2E test suites (`python tests/e2e/run_e2e_tests.py` and unit tests for Feature 12 & Feature 13).
5. Write your complete handoff report to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m4_1\handoff.md`.
6. Send a completion message to the orchestrator with your status and summary.
