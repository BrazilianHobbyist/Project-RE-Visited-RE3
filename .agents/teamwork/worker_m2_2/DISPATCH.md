## 2026-10-18T06:06:47Z
You are Worker M2 2 (teamwork_preview_worker).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m2_2\

Mandatory inputs to read before starting:
- Original Request: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md (READ THIS FIRST)
- Master Architecture: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- Explorer M2 R2 1 Report: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_r2_1\handoff.md
- Explorer M2 R2 2 Report: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_r2_2\handoff.md
- Explorer M2 R2 3 Report: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_r2_3\handoff.md
- Challenger M2 1 Defect Report: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m2_1\handoff.md
- Challenger M2 2 Defect Report: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m2_2\handoff.md

Files you own exclusively for writing:
- psxrecomp/runtime/src/re3_hd_backgrounds.cpp

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

Remediation Tasks in psxrecomp/runtime/src/re3_hd_backgrounds.cpp:
1. Fix LRU Eviction Zombie State (Challenger M2 1):
   In `upload_entry_to_gl()`, when an entry is evicted:
   ```cpp
   glDeleteTextures(1, &ent_it->second.gl_tex_id);
   s_metrics_evictions++;
   s_lru_list.erase(std::next(it).base());
   s_entries.erase(ent_it); // Erase from s_entries so returning to the room re-queues it!
   break;
   ```
2. Fix Re-queueing on Room Change (Challenger M2 1 & Explorer M2 R2 1):
   In `re3_hd_bg_on_room_change()`, update queue checks:
   `auto it = s_entries.find(k); if (it == s_entries.end() || it->second.state == BgState::None)`
   Ensure both prefetch cuts and active cut can be re-resolved and enqueued if evicted.
3. Fix Key=0 Sentinel Defect (Challenger M2 2 & Explorer M2 R2 2):
   In `worker_thread_func()`, replace `if (key == 0 || resolved_path.empty()) continue;`:
   Use an explicit `bool has_job = false;` flag and `uint32_t key = 0xFFFFFFFFu;` sentinel so that key 0 (`stage=0, room=0, cut=0`) is decoded properly and never dropped.
4. Fix LRU Recency Promotion on Cache Hits (Challenger M2 1 & Explorer M2 R2 3):
   In `re3_hd_bg_get_current_gl_tex()`, on cache hits (`it->second.gl_tex_id != 0`):
   ```cpp
   if (!s_lru_list.empty() && s_lru_list.front() != key) {
       s_lru_list.remove(key);
       s_lru_list.push_front(key);
   }
   ```

Empirical Verification Requirements:
1. Build runtime: `powershell -ExecutionPolicy Bypass -File "build.ps1"`
2. Re-compile and run Concurrency Stress Test:
   ```powershell
   clang++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_hd_backgrounds_stress.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -lpsapi -o build/test_re3_hd_backgrounds_stress.exe
   .\build\test_re3_hd_backgrounds_stress.exe
   ```
   Must pass 5/5 tests (including Test 4).
3. Re-compile and run Adversarial Test:
   ```powershell
   c++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m2_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -o build/test_re3_m2_adversarial.exe
   .\build\test_re3_m2_adversarial.exe
   ```
   Must pass 6/6 tests (including Test 3 for stage 0, room 0, cut 0).
4. Run Native Unit Test: `.\build\test_re3_hd_backgrounds_native.exe`
5. Run Full E2E Suite: `python tests/e2e/run_e2e_tests.py`

Write your handoff report to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m2_2\handoff.md` and notify the orchestrator via send_message.
