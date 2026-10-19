# BRIEFING — 2026-10-18T07:17:00Z

## Mission
Investigate Feature 13 (HD UI Sprite & Icon Replacements), hash-based identification (xxHash64), replacement path structure in mods/textures/<pack>/ui/sprites/, alpha cutout handling, and E2E test requirements.

## 🔒 My Identity
- Archetype: explorer
- Roles: investigator, synthesizer
- Working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_3
- Original parent: 7e5b962c-58f6-46df-a295-efcce3929cc7
- Milestone: M4 (Feature 13 - HD UI Sprite & Icon Replacements)

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Write only to our working directory: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m4_3\
- Synthesize findings into structured 5-component handoff report

## Current Parent
- Conversation ID: 7e5b962c-58f6-46df-a295-efcce3929cc7
- Updated: 2026-10-18T07:09:17Z

## Investigation State
- **Explored paths**:
  - `tests/e2e/test_tier1_features.py` (lines 544-578, `TestFeature13HdUiSprites`)
  - `tests/e2e/test_tier3_interactions.py` (lines 152-161, `test_i07_ui_sprite_and_actor_simultaneous_rendering`)
  - `tests/e2e/test_tier4_scenarios.py` (lines 180-210, `test_s05_full_inventory_and_status_cycle`)
  - `tests/e2e/harness/fixtures.py` (lines 31, 129-132)
  - `tests/e2e/harness/ui_pipeline.py` (lines 110-143, `bind_ui_sprite`)
  - `tests/e2e/harness/texture_pipeline.py` (lines 20-53, `compute_texture_xxhash64`, UV normalization)
  - `psxrecomp/runtime/include/re3_hd_textures.h` & `src/re3_hd_textures.cpp`
  - `psxrecomp/runtime/src/gpu.c` (lines 5060-5660, primitive execution)
  - `psxrecomp/runtime/src/gpu_gl_renderer.c` (lines 1700-1830 fragment shader, 3312-3365 `flush_tex_batch`, 3775-3784 hash bind hook, 3925-3945 `gpu_textured_rect`)
  - `psxrecomp/docs/STABLE_TEXTURE_FILTERING.md`
- **Key findings**:
  - Feature 13 tests verify: hash lookup, fallback on missing/unknown hash, caching of GL textures, disable toggle (`hd_ui=false`), and simultaneous rendering with 3D models.
  - Identification: xxHash64 over composite header (tpage, clut, u_min, v_min, u_max, v_max, flags) + palette + VRAM texels. Matches existing `re3_hd_tex_compute_from_vram`.
  - Path cascade: `mods/textures/<pack>/ui/sprites/<hash>.png` primary -> `ui/<hash>.png` fallback (E2E test harness compatibility) -> `replacements/<hash>.png` fallback.
  - Alpha cutouts: shader discard at `a < 0.01` ensures transparent bounding boxes never overwrite backdrops; `GL_CLAMP_TO_EDGE` prevents bleeding.
  - Sub-rectangle UV normalization: for sprites, normalized UVs must use `(uv - v_limits.xy) / (v_limits.zw - v_limits.xy + 1.0)`.
  - GPU hook: in `gpu_gl_renderer.c:3775-3784`, add branch for `!s_actor_prim_tag && g_re3_tex_config.hd_ui`.
- **Unexplored areas**: None within Feature 13 scope.

## Key Decisions Made
- Confirmed full design architecture and contract for Feature 13.
- Ready to write `handoff.md`.

## Artifact Index
- `DISPATCH.md` — incoming orders and timestamps
- `BRIEFING.md` — working memory and state
- `progress.md` — liveness heartbeat
- `handoff.md` — complete 5-component report
