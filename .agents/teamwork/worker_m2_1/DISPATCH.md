## 2026-10-18T05:32:44Z
You are Worker M2 1 (teamwork_preview_worker).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m2_1\

Mandatory inputs to read before starting:
- Original Request: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md (READ THIS FIRST)
- Master Architecture: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- Explorer M2 1 Report: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_1\handoff.md
- Explorer M2 2 Report: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_2\handoff.md
- Explorer M2 3 Report: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_3\handoff.md

Target Milestone: Milestone 2 — HD Pre-rendered Backgrounds Engine
Features owned: Features 2, 3, 4, 5, 6 from PROJECT.md Feature Inventory.

Files you own exclusively for writing:
- psxrecomp/runtime/include/re3_hd_backgrounds.h
- psxrecomp/runtime/src/re3_hd_backgrounds.cpp
- CMakeLists.txt (root)
- psxrecomp/runtime/src/re3_textures.cpp (init/shutdown hooks)
- psxrecomp/runtime/src/gpu_gl_renderer.c (camera state reading, rendering hook at backbuffer clear, occlusion mask helper)
- tests/ (native test files for verification)

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

Implementation Requirements:
1. `psxrecomp/runtime/include/re3_hd_backgrounds.h`:
   - Declare all C API functions with extern "C":
     - void re3_hd_bg_init(const char* pack_path);
     - void re3_hd_bg_shutdown(void);
     - void re3_hd_bg_on_room_change(uint8_t stage, uint8_t room, uint8_t cut);
     - bool re3_hd_bg_has_hd_current(void);
     - uint32_t re3_hd_bg_get_current_gl_tex(void);
     - bool re3_hd_bg_render_current(uint32_t fbo, int width, int height);
     - const char* re3_hd_bg_resolve_file(uint8_t stage, uint8_t room, uint8_t cut, char* out_path, size_t max_len);
     - void re3_hd_bg_get_screen_uv(int x, int y, int w, int h, float out_uv[4]);
2. `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`:
   - RESHDP resolver:
     - Check: R<stage><room:02X>_<cut:02X>.png, R<stage><room:02X>_<cut>.png, r<stage><room:02x>_<cut:02x>.png, r<stage><room:02x>_<cut>.png, and uppercase .PNG variations.
     - Prevent directory traversal (reject ..).
   - Async pre-fetcher worker thread:
     - std::mutex, std::condition_variable, std::thread, SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL).
     - When entering room, prefetch all cuts for (stage, room).
     - CRITICAL LINK CONSTRAINT: psx_window_icon.cpp defines STBI_NO_STDIO! Do NOT call stbi_load(filename). Read binary file via std::ifstream or fopen/fread into memory buffer and call stbi_load_from_memory().
   - Single-threaded GL constraint:
     - Do NOT call glGenTextures/glTexImage2D from worker thread. Produce decoded staging RGBA buffer in RAM, upload to GL texture on main render thread.
     - Free staging RAM immediately after glTexImage2D to maintain near-zero CPU RAM overhead.
   - LRU Cache:
     - 32-entry capacity with clean eviction via glDeleteTextures.
   - Screen-space UV mapping for occlusion masks:
     - u0 = (float)x / 320.0f, v0 = (float)y / 240.0f, u1 = (float)(x + w) / 320.0f, v1 = (float)(y + h) / 240.0f.
   - Transparent fallback:
     - Return false from re3_hd_bg_has_hd_current() when texture is missing or still decoding, allowing native PS1 VRAM rendering without dropping frames.
3. Integration:
   - Root CMakeLists.txt: add "${PSXRECOMP_ROOT}/runtime/src/re3_hd_backgrounds.cpp" to target_sources(RE3Recomp PRIVATE ...).
   - re3_textures.cpp: call re3_hd_bg_init(pack_path) in re3_textures_init, and re3_hd_bg_shutdown() in re3_textures_shutdown.
   - gpu_gl_renderer.c: in present and frame rendering, read 0x000D1F08, 0x000D1F09, 0x000D1F0A via psx_read_byte() and call re3_hd_bg_on_room_change; hook re3_hd_bg_render_current into backbuffer clear.
4. Empirical Verification:
   - Run runtime build: powershell -ExecutionPolicy Bypass -File "build.ps1"
   - Run E2E tests: python tests/e2e/run_e2e_tests.py --tier 1
   - Run native test: compile and run native tests for re3_hd_backgrounds.
   - Document build and test outputs verbatim in your handoff report.
