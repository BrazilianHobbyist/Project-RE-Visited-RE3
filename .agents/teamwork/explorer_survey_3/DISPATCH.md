## 2026-10-18T03:24:54Z
You are Explorer 3 (UI, Launcher & Configuration Explorer).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_3\
Project root: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp
Original request file: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md

MANDATORY FIRST STEP:
Read c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md.

YOUR MISSION:
Perform a deep technical survey of the codebase regarding Requirements R3, R5, and the Build Pipelines:
R3. HD User Interface & Menus (2D UI screen replacement for inventory, status, map, epilogues, clock in mods/textures/<pack>/ui/).
R5. Win32 Launcher Interface & Persistent Config ([textures] section in settings.toml, dedicated tab/section in Resident_Evil_3_Launcher.exe with pack selector, checkboxes for Backgrounds, Models, UI, and button to open mods folder).
Build pipelines: build.ps1 (runtime) and launcher/build.ps1 (launcher).

Key questions to answer:
1. How are 2D UI screens (inventory, status, map, epilogues, save screen, clock) rendered in the game? Are they full-screen quads, sprites, or TIM images? Where are they hooked in code?
2. How is settings.toml loaded and parsed in the main runtime? Which TOML library is used? What data structures hold active configuration?
3. What is the architecture of the Win32 launcher (`Resident_Evil_3_Launcher.exe`)? Where are its source files (look in `launcher/` or root)? How does it read and write `settings.toml`?
4. How to implement the new "Mods & Texturas HD" section/tab in the launcher, with installed pack dropdown/list from `mods/textures/`, checkboxes for Backgrounds, Models, UI, and "Abrir Pasta de Mods" (explorer.exe)?
5. How do `build.ps1` and `launcher/build.ps1` work? What compilers (clang, gcc, msvc), tools (cmake, ninja, etc.), or scripts are invoked? Does the project currently build cleanly?
