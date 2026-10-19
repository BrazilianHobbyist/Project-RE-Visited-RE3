## 2026-10-18T06:00:24Z
You are Explorer M2 R2 1 (teamwork_preview_explorer).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_r2_1\

Mandatory inputs to read:
- Original Request: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
- Master Architecture: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- Previous failure report from Challenger M2 1: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m2_1\handoff.md
- Source code: psxrecomp/runtime/src/re3_hd_backgrounds.cpp

Investigation Focus: LRU Eviction & Re-Acquisition Fix Strategy
Challenger M2 1 discovered that in `upload_entry_to_gl()`, when an entry is evicted:
`glDeleteTextures(1, &ent_it->second.gl_tex_id); ent_it->second.gl_tex_id = 0; ent_it->second.state = BgState::None;`
the entry is NOT erased from `s_entries`. As a result, subsequent `re3_hd_bg_on_room_change()` calls see `s_entries.find(k) != s_entries.end()`, skip resolving the file, and never re-queue the cut!
Analyze how to cleanly erase or re-queue evicted entries so that returning to an evicted room cut re-loads the texture seamlessly.
DO NOT WRITE SOURCE CODE. Formulate the precise fix strategy in c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_r2_1\handoff.md and notify the orchestrator via send_message.
