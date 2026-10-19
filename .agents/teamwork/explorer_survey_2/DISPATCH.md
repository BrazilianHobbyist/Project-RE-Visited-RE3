## 2026-10-18T03:24:54Z
Sender: 2974c4ea-67cd-482c-a523-a5380947c96d (parent)
Content:
You are Explorer 2 (3D Texture & Dumping Explorer).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_2\
Project root: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp
Original request file: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md

MANDATORY FIRST STEP:
Read c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md.

YOUR MISSION:
Perform a deep technical survey of the codebase regarding Requirements R2 and R4:
R2. 3D Model & Actor HD Texture Replacement (xxHash64 64-bit fingerprinting of textures, loading replacements from mods/textures/<pack>/replacements/<hash>.png, normalized UVs, up to 16x anisotropic + trilinear filtering, transparent fallback to original PS1 textures).
R4. Texture Dumping Tooling (configurable dump_textures = true to dump decoded actor/background textures to PNG with hashes and IDs).

Key questions to answer:
1. How does RE3Recomp decode, bind, and draw 3D polygon textures (actors: Jill, Nemesis, enemies, weapons, dynamic objects)?
2. Where in the graphics pipeline are PS1 VRAM/tpage/clut converted to OpenGL textures?
3. How should the 64-bit xxHash64 fingerprint be computed from the texture data/metadata (tpage, clut, width, height, raw pixels)? Is xxHash or similar already present in the codebase, or how should it be included?
4. How should UV coordinates be adjusted or normalized when binding a high-resolution replacement texture instead of the low-res PS1 sub-rectangle?
5. How is texture filtering configured? How to enable GL_TEXTURE_MAX_ANISOTROPY_EXT (up to 16x) and trilinear mipmapping (GL_LINEAR_MIPMAP_LINEAR)?
6. How to structure texture cache and fallback: how to cleanly fall back to PS1 textures when replacement PNG does not exist?
7. Where and how should `dump_textures = true` be hooked? How will PNG saving be implemented (e.g. stb_image_write)?

Deliverables:
Write a comprehensive survey report to:
c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_2\survey_textures.md
Also write handoff.md in your working directory.
When done, send a message to the orchestrator (caller) with your summary and file paths.
