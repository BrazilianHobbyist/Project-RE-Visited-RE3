# BRIEFING — 2026-10-18T06:08:00Z

## Mission
Analyze LRU Eviction & Re-Acquisition bug in psxrecomp/runtime/src/re3_hd_backgrounds.cpp and formulate a precise fix strategy so evicted background cuts are cleanly erased or re-queued when returning to them.

## 🔒 My Identity
- Archetype: teamwork_preview_explorer
- Roles: explorer, synthesizer
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_r2_1\
- Original parent: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Milestone: M2 R2 (LRU Eviction & Re-Acquisition Fix Strategy)

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Do NOT write source code (only write reports and analysis files in own folder)
- Produce structured 5-component handoff report (handoff.md)
- Send message to orchestrator via send_message when complete

## Current Parent
- Conversation ID: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Updated: not yet

## Investigation State
- **Explored paths**:
  - `psxrecomp/runtime/src/re3_hd_backgrounds.cpp` (LRU cache, eviction loop, worker thread, on_room_change, get_current_gl_tex)
  - `tests/test_re3_hd_backgrounds_stress.cpp` (Tests 1-5, particularly Test 4 LRU reacquisition)
  - `tests/test_re3_m2_adversarial.cpp` (Tests 1-6, particularly Test 3 boundary check on key == 0)
  - `.agents/teamwork/challenger_m2_1/handoff.md` (Challenger failure report)
  - `.agents/teamwork/ORIGINAL_REQUEST.md` & `PROJECT.md`
- **Key findings**:
  - Eviction in `upload_entry_to_gl()` sets `state = BgState::None` and deletes GL texture, but fails to erase the entry from `s_entries`.
  - When returning to an evicted cut, `re3_hd_bg_on_room_change()` skips resolution and queueing because `s_entries.find(key) != s_entries.end()`.
  - Cache hits in `re3_hd_bg_get_current_gl_tex()` fail to update `s_lru_list`, causing active/frequent textures to remain at the eviction tail.
  - Room prefetch loop queues all cuts to the back of `s_queue`, causing active cuts to experience priority inversion behind non-active room cuts.
  - Worker thread erroneously checks `if (key == 0 || resolved_path.empty()) continue;`, preventing Stage 0 Room 0 Cut 0 from ever decoding.
- **Unexplored areas**: None within Milestone 2 backgrounds subsystem scope.

## Key Decisions Made
- Formulated 5-Point Robust Strategy combining clean map erasure on eviction with dual-layer state guards, active cut priority queueing, LRU hit recency promotion, and worker key 0 fix.
- Written comprehensive 5-component handoff report at `handoff.md`.

## Artifact Index
- DISPATCH.md — dispatch message history
- BRIEFING.md — persistent working memory
- progress.md — liveness heartbeat
- handoff.md — 5-component handoff report for implementer/worker
