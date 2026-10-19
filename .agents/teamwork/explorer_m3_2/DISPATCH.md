# Dispatch: Explorer M3-2 (HD Texture Cache & OpenGL Pipeline Integration)

## Mission
Investigate HD texture loading, replacement caching, and OpenGL pipeline integration (`gpu_gl_renderer.c`) for Milestone 3.

## Authoritative Inputs
- ORIGINAL_REQUEST: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md`
- PROJECT: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md`
- Codebase paths:
  - `psxrecomp/runtime/src/gpu_gl_renderer.c`
  - `psxrecomp/runtime/src/re3_hd_backgrounds.cpp`
  - `psxrecomp/runtime/src/re3_textures.cpp`
  - `psxrecomp/runtime/include/re3_textures.h`

## Scope & Questions to Answer
1. How does `gpu_gl_renderer.c` currently compile and manage shaders (e.g. `TEX_FS`), uniform locations, and texture units?
2. How should texture unit 2 (`GL_TEXTURE2`) be used for HD replacement textures alongside unit 0 (`s_raw_tex`)?
3. How should `u_is_hd` uniform be declared in the fragment shader and controlled in C?
4. How should replacement PNGs from `mods/textures/<pack>/replacements/<hash>.png` (and uppercase/lowercase variations) be resolved and decoded? (e.g., using `stb_image.h`).
5. How should OpenGL texture parameters be configured: mipmap generation (`glGenerateMipmap`), trilinear filtering (`GL_LINEAR_MIPMAP_LINEAR`), and anisotropic filtering (`GL_TEXTURE_MAX_ANISOTROPY_EXT` up to 16x)?
6. How should the replacement texture cache (hash -> `GLuint`) and missing texture cache (hash -> negative cache) be designed in `re3_hd_textures.cpp` for zero stutter and clean lifecycle (`init` / `shutdown`)?
7. What is the transparent fallback behavior when a texture is missing or `hd_models` is disabled?

Write your comprehensive findings and recommendations to `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_2\handoff.md`.

## 2026-10-18T06:26:10Z
You are Explorer M3-2 (HD Texture Cache & OpenGL Pipeline Integration).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_2\
Please read:
- ORIGINAL_REQUEST: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
- PROJECT: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md
- DISPATCH: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_2\DISPATCH.md

Investigate replacement loading, texture caching, OpenGL shader branching (u_is_hd), texture unit 2 binding, trilinear mipmaps, and 16x anisotropic filtering.
Analyze gpu_gl_renderer.c, re3_hd_backgrounds.cpp, and re3_textures.cpp.
Propose the design and implementation for re3_hd_textures.h/.cpp and gpu_gl_renderer.c integration.
Write your complete findings and recommendations to c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_m3_2\handoff.md.
When finished, send a message to the orchestrator notifying completion.
