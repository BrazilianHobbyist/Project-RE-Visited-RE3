## 2026-10-18T05:18:38Z
You are Explorer M2 2 (teamwork_preview_explorer).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_2\
Mandatory inputs:
- Original Request: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md (READ THIS FIRST)
- Master Architecture: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- Previous survey: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_1\handoff.md

Target Milestone: Milestone 2 — HD Pre-rendered Backgrounds Engine
Investigation Focus: Asynchronous Pre-fetcher & LRU Cache Architecture
1. Investigate the existing async worker thread pattern in psxrecomp/runtime/src/hd_texture_pack.cpp:
   - Mutex, condition_variable, worker queue, decode pipeline.
   - stb_image usage: headers in psxrecomp/runtime/third_party/stb_image.h.
2. Design the async pre-fetching architecture for backgrounds:
   - Room-level pre-fetching: when camera enters a room (stage, room), eagerly pre-decode all cuts for that room (e.g. cut 00..15) into RAM or staging buffers in a background worker thread.
   - Main thread OpenGL texture upload: upload decoded RGBA buffers to GL textures on the render thread to avoid OpenGL context sharing complications.
   - LRU texture cache: cache up to N room cuts (e.g., 32 cuts) with clean eviction to prevent VRAM exhaustion.
3. Design transparent fallback:
   - If a background is not yet loaded or missing on disk, return false from re3_hd_bg_has_hd_current() so the engine falls back to original PS1 VRAM rendering seamlessly without stutter.
4. Document the thread safety, memory budget, and lifecycle in your handoff report.
DO NOT WRITE SOURCE CODE. Produce a detailed handoff report in c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_2\handoff.md and notify the orchestrator via send_message.
