# BRIEFING — 2026-10-18T06:05:00Z

## Mission
Analyze cache recency and eviction ordering in `re3_hd_bg_get_current_gl_tex()` and formulate a precise fix strategy for updating `s_lru_list` under strict thread safety without premature eviction of frequently viewed background cuts.

## 🔒 My Identity
- Archetype: explorer
- Roles: teamwork_preview_explorer
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m2_r2_3
- Original parent: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Milestone: M2 R2

## 🔒 Key Constraints
- Read-only investigation — do NOT implement / do NOT modify source code
- Formulate precise fix strategy in handoff.md
- Thread safety analysis under mutex locks
- Comply with project architecture and coding standards

## Current Parent
- Conversation ID: 55deb21e-cccb-4bc3-8785-823c6ac705db
- Updated: not yet

## Investigation State
- **Explored paths**:
  - `psxrecomp/runtime/src/re3_hd_backgrounds.cpp` (lines 1-739): LRU list management, upload mechanics, mutex locking, eviction loop.
  - `psxrecomp/runtime/include/re3_hd_backgrounds.h`: API contracts.
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`: Render hook callsite (`gpu_gl_renderer.c:3955`).
  - `tests/test_re3_hd_backgrounds_stress.cpp`: LRU eviction test case 4.
  - `tests/test_re3_m2_adversarial.cpp`: Adversarial test cases and zero-key edge case.
  - `.agents/teamwork/challenger_m2_1/handoff.md`: Concurrency & stress findings.
  - `.agents/teamwork/challenger_m2_2/handoff.md`: Edge-case resilience findings.
- **Key findings**:
  - `re3_hd_bg_get_current_gl_tex()` never updates `s_lru_list` on cache hits (`gl_tex_id != 0`), degrading the cache from true LRU to insertion-order FIFO.
  - Frequently viewed camera cuts stay at the tail of `s_lru_list` and are prematurely evicted when new cuts are uploaded.
  - `s_mutex` is already acquired via `std::lock_guard<std::mutex> lock(s_mutex)` inside `re3_hd_bg_get_current_gl_tex()`, ensuring 100% thread safety for `s_lru_list` updates with zero lock overhead.
  - Adding an O(1) MRU guard `if (s_lru_list.empty() || s_lru_list.front() != key)` eliminates per-frame list traversal and heap allocations across 99.9% of gameplay frames.
- **Unexplored areas**: None within the scope of Cache Recency & Eviction Ordering.

## Key Decisions Made
- Recommended Approach 1: Guarded `remove` + `push_front` under `s_mutex`. Provides optimal balance of thread safety, zero heap churn, zero iterator invalidation risk, and complete self-healing guarantees.

## Artifact Index
- DISPATCH.md — Initial dispatch prompt
- BRIEFING.md — Persistent context & state
- progress.md — Liveness heartbeat
- handoff.md — 5-component handoff report
