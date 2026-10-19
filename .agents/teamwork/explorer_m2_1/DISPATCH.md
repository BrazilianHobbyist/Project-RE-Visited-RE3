## 2026-10-18T05:18:38Z
You are Explorer M2 1 (teamwork_preview_explorer).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_1\
Mandatory inputs:
- Original Request: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md (READ THIS FIRST)
- Master Architecture: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- Previous survey: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_1\handoff.md

Target Milestone: Milestone 2 — HD Pre-rendered Backgrounds Engine
Investigation Focus: State Tracking & RESHDP File Resolution
1. Analyze memory addresses:
   - 0x000D1F08 (Stage)
   - 0x000D1F09 (Room)
   - 0x000D1F0A (Cut)
   Inspect how psx_read_byte() reads from guest RAM in psxrecomp/runtime/src/gpu_gl_renderer.c and memory.c.
2. Investigate RESHDP file naming conventions in mods/textures/<pack>/backgrounds/:
   - Canonical format: R<stage><room:02X>_<cut:02X>.png (e.g., R100_00.png, R20A_03.png)
   - Alternative variations: single-digit cuts (R100_0.png), lowercase formats (r100_00.png), decimal vs hex room numbers.
   - Design a fast, deterministic file resolver that checks candidate paths and handles fallback gracefully.
3. Define the exact API functions and data structures for re3_hd_backgrounds.h:
   - re3_hd_bg_init(const char* pack_path)
   - re3_hd_bg_shutdown(void)
   - re3_hd_bg_on_room_change(uint8_t stage, uint8_t room, uint8_t cut)
   - re3_hd_bg_has_hd_current(void)
   - re3_hd_bg_get_current_gl_tex(void)
   - re3_hd_bg_render_current(uint32_t fbo, int width, int height)
4. Document the exact call graph and interfaces in your handoff report.
DO NOT WRITE SOURCE CODE. Produce a detailed handoff report in c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_1\handoff.md and notify the orchestrator via send_message.
