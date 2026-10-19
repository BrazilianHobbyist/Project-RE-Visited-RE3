# Handoff Report — Explorer 3 (UI, Launcher & Configuration Explorer)

## 1. Observation

1. **Launcher Source & Architecture**:
   - File: `launcher/main.cpp` (2,669 lines, 142,930 bytes).
   - Lines 12–19: Color palette constants (`CLR_BG = RGB(22, 22, 26)`, `CLR_TEXT = RGB(225, 225, 230)`, `CLR_ACCENT = RGB(200, 40, 40)`).
   - Lines 34–72: `enum ControlID` defining controls up to `ID_CHK_FAST_DOORS = 1037`.
   - Lines 597–617: `struct GameConfig` storing current settings.
   - Lines 621–687: `GameConfig LoadConfig()` reads `settings.toml` via `std::wifstream`, parsing line-by-line (`key = val`).
   - Lines 689–785: `bool SaveConfig(const GameConfig& cfg)` writes `settings.toml` via `std::wofstream`, structured with sections `[general]`, `[video]`, `[audio]`, `[controller]`, `[cheats]`.
   - Lines 1713–1755: `void ShowCheatsDialog(HWND hWndParent)` creates modal dialog `g_hWndCheats` (`RE3CheatsWindowClass`, 580x470 px).
   - Lines 2371–2373: `CreateWindowW(L"BUTTON", L"Door Skipping (Fast Room Transitions)", ...)` and `SendMessageW(..., BM_SETCHECK, ...)` showing control creation.
   - Lines 2590–2659: Window registration (`cwc`, `swc`, `chwc`, `wc`) and main window creation (`RECT rcMain = { 0, 0, 560, 808 };`).

2. **Launcher Build Script**:
   - File: `launcher/build.ps1` (41 lines).
   - Invokes `windres "$ScriptDir\resource.rc" -O coff -o "$ScriptDir\resource.o"`.
   - Invokes `g++ -O2 -static -mwindows -municode "$ScriptDir\main.cpp" "$ScriptDir\resource.o" -o "$ScriptDir\Resident_Evil_3_Launcher.exe" -lgdiplus -lcomctl32 -lcomdlg32 -lshlwapi -luxtheme`.
   - Command executed: `powershell -ExecutionPolicy Bypass -File "launcher/build.ps1"`. Result: exit code 0.
   - Verbatim output:
     ```
     Compilando recursos do Launcher...
     Compilando executavel estatico do Launcher...
     Launcher compilado com sucesso: C:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\launcher\Resident_Evil_3_Launcher.exe
     Copiado para: C:\Users\victo\Documents\Projetos\Decomp - SOTC\ResidentEvil3-PCPort\Resident_Evil_3_Launcher.exe
     Sincronizado com: C:\Users\victo\Documents\Projetos\Decomp - SOTC\launcher
     ```

3. **Runtime Settings Subsystem**:
   - File: `psxrecomp/recompiler/src/config_loader.cpp` (lines 24–26): `#include "toml.hpp"` (`toml11` library).
   - File: `psxrecomp/recompiler/src/config_loader.h` (lines 1358–1538): `struct UserSettings` with fields for `[video]`, `[audio]`, `[controller]`, etc.
   - File: `psxrecomp/runtime/src/main.cpp` (lines 5691–5788): `static void load_re3_qol_settings(const std::filesystem::path& path)` implements a secondary line-by-line parser for RE3-specific settings (`fast_doors`, cheats, aspect ratios).
   - Lines 15847–15848: `PSXRecompV4::UserSettings us = PSXRecompV4::load_user_settings(settings_path);`
   - Lines 15926 and 17452: `load_re3_qol_settings(settings_path);`

4. **2D Screen & UI Rendering Architecture**:
   - File: `psxrecomp/runtime/src/gpu.c`:
     - Lines 5241–5275: `is_3d_actor_polygon`, `note_3d_actor_polygon`, `mod_texture_packet_bank` in `gp0_exec_shaded_textured_tri()`.
     - Lines 5404–5442: `gp0_exec_mono_rect()`.
     - Lines 5835–5850: `gp0_exec_cpu_to_vram()` handling GP0 0xA0 (`LoadImage`).
   - File: `psxrecomp/runtime/src/gpu_gl_renderer.c`:
     - Lines 367–385: Dual scale system (`s_hr_scale` for `s_hr_fbo` and `s_out_scale` for presented window).
     - Line 568: `static GLuint s_hr_tex = 0, s_hr_fbo = 0, s_hr_rb = 0;`.
     - Lines 8157–8339: `gl_renderer_present_vram(...)` presenting `s_hr_fbo` / `s_hr_tex` to the display.
   - File: `psxrecomp/runtime/src/main.cpp`:
     - Lines 5272–5310: `struct RE3ItemSlot` and `re3_get_item_name()` operating on RAM inventory offsets `0x000D2134` (items) and `0x000D225E` (max slots 8 or 10).
     - Lines 5558–5562: Secret mode flags (`0x000D1F32`) and epilogues gallery count (`ram[0x000D1F3E] = 8`).
     - Line 8617: `re3_maintain_qol_per_frame();` called every frame inside the main loop.

5. **Runtime Build Script**:
   - File: `build.ps1` (127 lines).
   - Invokes `cmake -S "$ProjectRoot" -B "$BuildDir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` and `cmake --build "$BuildDir" --config Release -- -j <logical_cores>`.
   - Command executed: `powershell -ExecutionPolicy Bypass -File "build.ps1"`. Result: exit code 0.
   - Verbatim output:
     ```
     [CONFIG]  Configurando CMake...
     ...
     [BUILD] Compilando...
     [6/7] Linking CXX executable Resident_Evil_3__Nemesis_Recompiled.exe
     [OK] Build concluído!
        Executável: C:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\build\Resident_Evil_3__Nemesis_Recompiled.exe
     ```

## 2. Logic Chain

1. **UI Screens Hooking (Observation 4)**:
   - On the PS1 GPU, 2D screens (inventory, status, map, epilogues, save, clock) are rendered using a combination of `POLY_FT4` quads, `SPRT` textured rectangles, and `LoadImage` transfers to VRAM.
   - The PSXRecomp OpenGL backend renders all primitives directly into the high-resolution framebuffer `s_hr_fbo`.
   - Because the main loop in `main.cpp` executes `re3_maintain_qol_per_frame()` every frame with direct access to PS1 RAM (`g_psx_ram`), the engine can determine the active game state and screen mode (via RAM offsets `0x000D1F08` Stage, `0x000D1F09` Room, `0x000D1F0A` Cut, `0x000D1F3E` Epilogues, `0x000D225E` Inventory).
   - Therefore, a dual-tier strategy is optimal: (1) Replacing base full-screen menu quads/rectangles with 1080p/4K backdrops from `mods/textures/<pack>/ui/<screen>.png` directly in `s_hr_fbo`, while (2) using xxHash64 GPU texture replacement for individual UI sprites, icons, and fonts (`mods/textures/<pack>/ui/<hash>.png` and `mods/textures/<pack>/replacements/<hash>.png`).

2. **Configuration Persistence in `settings.toml` (Observations 1 & 3)**:
   - The Launcher (`launcher/main.cpp`) reads `settings.toml` using `LoadConfig()` and writes it using `SaveConfig()`.
   - The runtime (`main.cpp`) reads `settings.toml` using `load_user_settings()` and `load_re3_qol_settings()`.
   - Adding a `[textures]` section (`pack`, `hd_backgrounds`, `hd_models`, `hd_ui`, `dump_textures`) in `settings.toml` fits naturally into `LoadConfig()` and `SaveConfig()` in `launcher/main.cpp` and `load_re3_qol_settings()` in `runtime/src/main.cpp` without breaking upstream `toml11` parsing.

3. **Launcher "Mods & Texturas HD" Interface (Observations 1 & 2)**:
   - The launcher already has a proven modal dialog pattern: `ShowCheatsDialog` / `RE3CheatsWindowClass` (580x470 px).
   - A dedicated dialog `ShowTexturesDialog` (`RE3TexturesWindowClass`, 580x420 px) triggered by a button `g_hBtnOpenTextures` ("🎨 Mods & Texturas HD...") provides the cleanest UX without increasing the main window's 808px height.
   - Folder enumeration in `<buildDir>\mods\textures` using Win32 `FindFirstFileW` / `FindNextFileW` cleanly populates the pack combobox.
   - Opening the folder in Windows Explorer is natively achieved via `ShellExecuteW(NULL, L"open", modsPath.c_str(), NULL, NULL, SW_SHOWNORMAL)`.

4. **Build Pipelines Feasibility (Observations 2 & 5)**:
   - Both `launcher/build.ps1` and `build.ps1` compiled with exit code 0 and produced working executables without warnings or errors.
   - The build toolchains (MinGW GCC for launcher, CMake + Ninja + Clang/GCC for runtime) are fully configured and functional on this system.

## 3. Caveats

- **Network Mode**: Investigation was performed in local environment with local toolchains and web searches for community pack conventions (Seamless HD Project / RESHDP).
- **Submodule Edits**: The `psxrecomp` submodule has existing uncommitted modifications in `runtime/src/gpu_gl_renderer.c`, `runtime/src/main.cpp`, `runtime/src/gpu.c`, which match `patches/psxrecomp_runtime.patch`. New texture subsystem changes must either be added directly or maintained in a modular header/source (`re3_textures.h` / `re3_textures.c`) to preserve patch cleanly.
- No other caveats.

## 4. Conclusion

- **Requirement R3 (HD UI & Menus)** is fully achievable via a dual-tier approach: intercepting full-screen menu quads in `gpu.c` to draw HD PNG backdrops from `mods/textures/<pack>/ui/` into `s_hr_fbo`, complemented by xxHash64 hash-based sprite/icon replacement.
- **Requirement R5 (Launcher & Config)** is straightforward to implement: extend `GameConfig` in `launcher/main.cpp`, add `[textures]` section to `settings.toml`, and implement `ShowTexturesDialog` with dynamic directory enumeration of `mods/textures/` and `ShellExecuteW` explorer launch.
- **Build Pipelines**: Both `build.ps1` and `launcher/build.ps1` currently build cleanly with exit code 0.

## 5. Verification Method

1. **Verify Launcher Build**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "launcher/build.ps1"
   ```
   *Expected outcome*: Exit code 0, generates `launcher/Resident_Evil_3_Launcher.exe`.
2. **Verify Runtime Build**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "build.ps1"
   ```
   *Expected outcome*: Exit code 0, generates `build/Resident_Evil_3__Nemesis_Recompiled.exe`.
3. **Verify Settings Persistence**:
   - Inspect `build/settings.toml`.
   - Verify parsing in `launcher/main.cpp` lines 621–785 and `psxrecomp/runtime/src/main.cpp` lines 5691–5788.
4. **Inspect Survey Report**:
   - Read `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\explorer_survey_3\survey_launcher_ui.md`.
