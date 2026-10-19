# Dispatch History

## 2026-10-18T03:23:26Z

You are the Project Orchestrator (teamwork_preview_orchestrator).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\orchestrator_1\
Project root: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp
Original request file: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md

Your mission:
Implement complete native support for high-definition (HD) texture and 3D model packs in the statically recompiled port of Resident Evil 3: Nemesis (RE3Recomp / Project RE: Visited), with hybrid support for Seamless HD Project (RESHDP) format and GPU hash-based texture replacement.

Core Requirements:
R1. HD Pre-rendered Backgrounds Engine (OpenGL background loader using stage/room/cut memory states 0x000D1F08, 0x000D1F09, 0x000D1F0A, async/cached loading from mods/textures/<pack>/backgrounds/ with RESHDP naming R<stage><room>_<cut>.png, rendering to s_hr_fbo with pixel-perfect alignment to occlusion masks).
R2. 3D Model & Actor HD Texture Replacement (xxHash64 64-bit fingerprinting of textures, loading replacements from mods/textures/<pack>/replacements/<hash>.png, normalized UVs, up to 16x anisotropic + trilinear filtering, transparent fallback to original PS1 textures).
R3. HD User Interface & Menus (2D UI screen replacement for inventory, status, map, epilogues, clock in mods/textures/<pack>/ui/).
R4. Texture Dumping Tooling (configurable dump_textures = true to dump decoded actor/background textures to PNG with hashes and IDs).
R5. Win32 Launcher Interface & Persistent Config ([textures] section in settings.toml, dedicated tab/section in Resident_Evil_3_Launcher.exe with pack selector, checkboxes for Backgrounds, Models, UI, and button to open mods folder).

Strict Acceptance Criteria:
- HD backgrounds load without stutter/lag during camera angle changes in continuous gameplay.
- Foreground occlusion masks maintain correct clipping over 3D actors.
- 3D actor HD textures render without UV distortion, seams, or edge artifacts; anisotropic/trilinear filtering active at stable 30 FPS; fallback to PS1 textures 100% functional.
- dump_textures = true dumps valid PNG files.
- Launcher lists packs in mods/textures/ and persists selections to settings.toml.
- Both build.ps1 (runtime) and launcher/build.ps1 (launcher) compile cleanly without errors or critical warnings.

Engineering Guidelines & Governance:
- Follow Level 2 rigor (Pre-Flight baseline check, end-to-end call-graph validation, zero silent omissions, atomic changes, persistent state tracking in PROGRESS.md).
- Create your BRIEFING.md and progress.md in your working directory (c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\orchestrator_1\). Keep them updated at each milestone.
- Dispatch specialist subagents as needed (explorers, implementers, reviewers) following subagent directory conventions.
- When all requirements are met and verified, report completion to the Sentinel.
