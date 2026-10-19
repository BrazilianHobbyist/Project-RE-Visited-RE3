# BRIEFING — 2026-10-18T05:56:45Z

## Mission
Adversarial concurrency & stress testing of RE3 HD Background subsystem (M2): high-frequency room/cut switches, worker thread decoding vs main thread upload races, LRU cache eviction boundary, rapid init/shutdown cycles, zero crashes, zero data races, memory bounding.

## 🔒 My Identity
- Archetype: teamwork_preview_challenger
- Roles: critic, specialist
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m2_1\
- Original parent: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Milestone: M2
- Instance: 1 of 1

## 🔒 Key Constraints
- Review-only — do NOT modify implementation code
- Report any failures as findings — do NOT fix them yourself
- Empirical verification required: write and execute test harnesses, don't rely on worker claims
- Adhere strictly to project governance and layout rules (.agents/teamwork/ holds metadata only, tests go in proper source/test locations or build directories)

## Current Parent
- Conversation ID: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Updated: 2026-10-18T05:51:09Z

## Review Scope
- **Files to review**:
  - `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
  - `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
  - `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\worker_m2_1\handoff.md`
  - `psxrecomp/runtime/include/re3_hd_backgrounds.h`, `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`
  - `tests/test_re3_hd_backgrounds_stress.cpp`
- **Interface contracts**: PROJECT.md, ORIGINAL_REQUEST.md
- **Review criteria**: Concurrency safety, race-free worker thread decode vs main thread query/upload, LRU eviction correctness (>32 entries, pinning active textures, re-acquisition of evicted textures), init/shutdown reentrancy/leak safety, memory bounds, stability.

## Key Decisions Made
- Designed and compiled standalone adversarial C++ stress test harness `tests/test_re3_hd_backgrounds_stress.cpp` linked with SDL3, OpenGL, and psapi.
- Tested: 200 rapid init/shutdown cycles, 1000 concurrent room/cut changes across 10 threads, worker decode vs main query races, LRU eviction boundary (>32 entries) with re-acquisition of evicted textures, and memory boundedness.
- Empirically reproduced critical functional failure: Evicted textures are permanently rendered inaccessible upon re-visiting due to `s_entries` lookup retaining `BgState::None` without re-queueing or erasing.
- Ruled verdict: REPORT FAILURE.

## Artifact Index
- `tests/test_re3_hd_backgrounds_stress.cpp` — Native adversarial stress test harness
- `build/test_re3_hd_backgrounds_stress.exe` — Compiled executable
- `.agents/teamwork/challenger_m2_1/handoff.md` — 5-component handoff report

## Attack Surface
- **Hypotheses tested**:
  1. Concurrency on `re3_hd_bg_on_room_change`: 10 threads, 1000 changes -> Passed without crash or deadlocks.
  2. Race between worker decode and main thread queries -> Passed without crash or memory corruption.
  3. Rapid init/shutdown reentrancy (200 cycles) -> Passed with clean thread joining and zero leaks.
  4. Memory bounding across 1000 cut changes -> Passed (diff < 50MB, observed +12 KB).
  5. LRU cache eviction boundary & re-acquisition -> FAILED! Evicted cuts are never reloaded upon re-visiting.
- **Vulnerabilities found**:
  1. [CRITICAL] Permanent texture starvation on evicted cuts: Upon eviction in `upload_entry_to_gl`, entry state is set to `BgState::None` but entry is NOT erased from `s_entries`. In `re3_hd_bg_on_room_change`, lines 602 & 624 check `s_entries.find(...) == s_entries.end()`, which skips re-queueing. Cut is never re-decoded or re-uploaded.
  2. [MEDIUM] LRU list not refreshed on cache hits: `s_lru_list.remove/push_front` only happens during initial GL upload. Subsequent access to already resident GL textures does not refresh recency, violating the LRU contract.
  3. [LOW] `s_entries` metadata memory leak: Because evicted entries are never erased from `s_entries`, metadata grows unbounded with every unique camera cut visited.
- **Untested angles**:
  - Full game playthrough across 50+ hours.

## Loaded Skills
- None
