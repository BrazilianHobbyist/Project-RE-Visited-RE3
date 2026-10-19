## 2026-10-18T05:18:38Z

You are Explorer M2 3 (teamwork_preview_explorer).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_3\
Mandatory inputs:
- Original Request: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md (READ THIS FIRST)
- Master Architecture: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- Previous survey: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_1\handoff.md

Target Milestone: Milestone 2 — HD Pre-rendered Backgrounds Engine
Investigation Focus: OpenGL FBO Composition & Occlusion Mask Alignment
1. Analyze psxrecomp/runtime/src/gpu_gl_renderer.c:
   - Lines 568, 4652-4675: s_hr_fbo, s_hr_tex, s_hr_rb allocation and binding.
   - Lines 1970-1980: hr_begin(clip_to_draw_area) setup.
   - Lines 8150-8340: Frame presentation in gl_renderer_present_vram().
   - Determine the exact hook point where the HD background quad must be drawn into s_hr_fbo (at frame start or when backbuffer is initialized before 3D actors).
2. Analyze occlusion mask quad alignment:
   - How does RE3 render foreground masks? (OT depth-sorted POLY_FT4 quads).
   - In 4K / high-resolution presentation, how to ensure foreground mask quads sample the HD texture in screen-space coordinates (u = x / width, v = y / height) to achieve seamless 1:1 pixel alignment over 3D actors?
   - Handling aspect ratios (4:3 original framing vs widescreen options in settings.toml).
3. Build system integration:
   - CMakeLists.txt changes: add re3_hd_backgrounds.cpp to target_sources.
   - Verify compiler flags, includes, and dependencies.
DO NOT WRITE SOURCE CODE. Produce a detailed handoff report in c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_3\handoff.md and notify the orchestrator via send_message.
