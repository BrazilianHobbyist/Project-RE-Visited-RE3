# Dispatch: Explorer M3-1 (xxHash64 Fingerprinting & Texture Payload)

## Mission
Investigate xxHash64 fingerprinting and texture payload extraction for 3D model & actor HD textures in RE3Recomp per Milestone 3.

## Authoritative Inputs
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- Codebase paths:
  - `psxrecomp/runtime/src/gpu.c`
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`
  - `psxrecomp/runtime/src/re3_textures.cpp`
  - `psxrecomp/runtime/include/re3_textures.h`
  - `psxrecomp/runtime/include/re3_hd_textures.h` (to specify)

## Scope & Questions to Answer
1. How does RE3Recomp process textured polygons in `gpu.c` and render them in `gpu_gl_renderer.c`?
2. Exactly how are `tpage`, `clut`, UV bounding box (`lim[4]`), palette data, and VRAM texels extracted for actor polygons?
3. What is the exact composite data buffer format for `xxHash64` fingerprinting that ensures unique and deterministic identification of 3D actor textures?
4. How should `xxhash.h` (`XXH_INLINE_ALL`) be integrated into `psxrecomp/runtime/include/xxhash.h`?
5. Propose the exact implementation design and function signatures for `re3_hd_tex_compute_hash(...)`.

Write your comprehensive findings and recommendations to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_1\handoff.md`.

## 2026-10-18T06:26:10Z
[Message] sender=7e5b962c-58f6-46df-a295-efcce3929cc7 priority=MESSAGE_PRIORITY_HIGH
You are Explorer M3-1 (Fingerprinting & Texture Payload).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_1\
Please read:
- ORIGINAL_REQUEST: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
- PROJECT: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- DISPATCH: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_1\DISPATCH.md

Investigate xxHash64 fingerprinting and texture payload extraction for 3D model & actor HD textures in RE3Recomp.
Analyze gpu.c, gpu_gl_renderer.c, vram.c, and existing texture infrastructure.
Propose the exact hashing algorithm, data buffer composition, xxhash.h integration, and C/C++ function signatures.
Write your complete findings and recommendations to c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_1\handoff.md.
When finished, send a message to the orchestrator notifying completion.
