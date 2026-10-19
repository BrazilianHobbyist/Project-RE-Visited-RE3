# Progress — Explorer M4-3 (Feature 13: HD UI Sprite & Icon Replacements)

- Last visited: 2026-10-18T07:17:00Z
- Status: Investigation Completed
- Current Step: Writing handoff report and preparing orchestrator notification
- Findings:
  - E2E Test Suite structure and verification logic analyzed across tiers 1, 3, 4.
  - Identification mechanism: deterministic 64-bit xxHash64 (seed 0) on composite payload (metadata header + CLUT palette + VRAM texels within bounding box).
  - Directory path cascade established: `mods/textures/<pack>/ui/sprites/<hash>.png` -> fallback `ui/<hash>.png` -> fallback `replacements/<hash>.png`.
  - Alpha cutout handling: fragment shader culls transparent background via `discard` (`a < 0.01`), with bilinear filtering and `GL_CLAMP_TO_EDGE` preventing border bleeding.
  - Sub-rectangle UV normalization: critical difference from full-page 3D models identified (`v_limits` normalization).
  - Integration hooks mapped in `gpu.c` (primitives 0x2C..0x2F quads, 0x64/0x65 rects, 0x74/0x75, 0x7C/0x7D) and `gpu_gl_renderer.c:3775-3784`.
