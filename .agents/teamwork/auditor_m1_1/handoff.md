# Forensic Audit Report: Milestone 1 (Settings & Config Engine)

**Auditor**: Forensic Auditor 1 (`auditor_m1_1`)  
**Working Directory**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\auditor_m1_1\`  
**Target Milestone**: M1 (Settings & Config Engine)  
**Profile**: General Project (Demo Mode)  
**Date**: 2026-10-18  
**Verdict**: **CLEAN**

---

## 1. Observation

1. **Source Code Inspection**:
   - `psxrecomp/runtime/include/re3_textures.h` (lines 1-37):
     Declares `re3_texture_config_t` with members `pack[128]`, `hd_backgrounds`, `hd_models`, `hd_ui`, and `dump_textures`. Declares `extern re3_texture_config_t g_re3_tex_config;`, `re3_textures_init()`, `re3_textures_shutdown()`, and `re3_textures_get_config()` with `extern "C"` linkage, matching `PROJECT.md § Interface Contracts` exactly.
   - `psxrecomp/runtime/src/re3_textures.cpp` (lines 1-167):
     Implements configuration defaults, path normalization (`normalize_path_string`), and authentic filesystem checks via `std::filesystem::exists`, `std::filesystem::is_directory`, and `std::filesystem::create_directories`. Path resolution (`re3_textures_resolve_path`) verifies disk presence and handles `.png` / `.PNG` case fallback.
   - `psxrecomp/runtime/src/main.cpp`:
     - Line 59: `#include "re3_textures.h"`.
     - Lines 5692-5785: In `load_re3_qol_settings()`, opens `std::ifstream f(path)` on disk. Parses `pack`, `hd_backgrounds`, `hd_models`, `hd_ui`, and `dump_textures` dynamically from `settings.toml`, writing to `g_re3_tex_config` and printing diagnostics to `stdout`.
     - Line 15952-15953: On cold boot, executes `load_re3_qol_settings(settings_path);` followed by `re3_textures_init(exe_dir_from_argv(argv[0]).string().c_str());`.
     - Line 17479-17480: On session reboot/restart, executes `load_re3_qol_settings(exe_dir_from_argv(argv[0]) / "settings.toml");` followed by `re3_textures_init(exe_dir_from_argv(argv[0]).string().c_str());`.
     - Line 19617: At normal exit, executes `re3_textures_shutdown();` immediately prior to `SDL_Quit()`.
   - `CMakeLists.txt` (lines 35-37):
     `target_sources(RE3Recomp PRIVATE "${PSXRECOMP_ROOT}/runtime/src/re3_textures.cpp")` integrates the compilation into the main executable target.

2. **Empirical Build Execution**:
   - Running `powershell -ExecutionPolicy Bypass -File "build.ps1"`:
     ```
     [CONFIG] Configurando CMake...
     -- psxrecomp: SDL backend = SDL3
     [BUILD] Compilando...
     [6/7] Linking CXX executable Resident_Evil_3__Nemesis_Recompiled.exe
     [OK] Build concluído!
        Executável: C:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\build\Resident_Evil_3__Nemesis_Recompiled.exe
     ```
     Exit code: `0`. Binary generated authentically.

3. **Empirical Test Suite Execution**:
   - Running `python -m unittest tests.e2e.test_tier1_features tests.e2e.test_tier2_boundaries`:
     ```
     Ran 120 tests in 3.863s
     OK
     ```
     Exit code: `0`. 120/120 tests passed.
   - Running native C++ unit tests:
     ```
     clang++ -std=c++17 -I psxrecomp/runtime/include tests/test_re3_textures_native.cpp psxrecomp/runtime/src/re3_textures.cpp -o build/test_re3_textures_native.exe
     .\build\test_re3_textures_native.exe
     ```
     Output:
     ```
     Running native re3_textures tests...
     [PASS] test_default_config
     [PASS] test_init_with_null_and_empty
     [PASS] test_pack_resolution
     All native re3_textures tests passed successfully!
     ```
     Exit code: `0`.

4. **Forensic Adversarial Stress Testing**:
   - Executed an independent test harness challenging:
     - Missing `settings.toml` (graceful handling, leaves defaults).
     - Indented comments, mixed casing (`PACK`, `HD_BACKGROUNDS`, `hd_MODELS`), whitespace padding, and inline quotes.
     - Buffer overflow resilience against 500-character pack names (properly bounded by `sizeof(pack) - 1` with guaranteed null termination).
     - Directory resolution, case sensitivity (`.png` <-> `.PNG`), null output buffer pointers, zero lengths, and null filenames.
     - Rapid multiple `init` and `shutdown` cycles (no resource leaks, clean idempotency).
   - Result: All adversarial test cases passed with exit code `0`.

---

## 2. Logic Chain

1. **No Hardcoded Test Results**:
   - The settings loader in `main.cpp` parses arbitrary key-value streams from `std::ifstream` rather than returning fixed values. Tests supply varied, dynamically generated configurations and assert outputs derived from file content.
2. **No Dummy or Facade Implementations**:
   - `re3_textures_init` performs genuine filesystem checks on `mods/textures/<pack>` via `std::filesystem::is_directory`. If absent, it sets `s_pack_valid = false` and informs the runtime.
   - `re3_textures_resolve_path` only returns `true` when a candidate file actually exists as a regular file on disk (`!is_directory`).
3. **Authentic File Reading**:
   - The call graph directly links `settings_path` to `load_re3_qol_settings()`, opening `std::ifstream f(path)` at line 5693. File content is read line by line.
4. **Authentic Configuration State**:
   - `g_re3_tex_config` is declared as an `extern` global C struct, backed by storage in `re3_textures.cpp`. Ingested keys directly mutate `g_re3_tex_config`, and downstream subsystems access it via `re3_textures_get_config()`.
5. **Standard Filesystem Compliance**:
   - All filesystem operations utilize modern `std::filesystem` APIs with `std::error_code` overloads to prevent unhandled exceptions during runtime path queries.

---

## 3. Caveats

- Milestone 1 establishes the configuration and path resolution substrate. The downstream rendering and dumping consumers (Milestones 2 through 6) will link against these accessors in subsequent phases.
- Pre-existing upstream CMake warnings concerning stale BIOS generated stamps (`generated/OpenBIOS_*.c` and `generated/SCPH1001_*.c`) are present in the repository baseline and unrelated to Milestone 1 deliverables.

---

## 4. Conclusion

**Verdict: CLEAN**

Milestone 1 satisfies all integrity constraints under Demo Mode:
- Zero hardcoded test outputs.
- Zero facade or dummy functions.
- Zero fabricated verification logs or artifacts.
- Genuine `settings.toml` disk stream parsing.
- Genuine `std::filesystem` path resolution.
- Clean compilation and 100% test pass rate across unit, E2E, and adversarial test suites.

The Milestone 1 work product is **approved** for merge and progression to Milestone 2.

---

## 5. Verification Method

To independently verify this audit verdict:

1. **Verify Runtime Build**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File "build.ps1"
   ```
   *Expected outcome*: Exit code 0; `build\Resident_Evil_3__Nemesis_Recompiled.exe` built.

2. **Run E2E Feature & Boundary Tests**:
   ```powershell
   python -m unittest tests.e2e.test_tier1_features tests.e2e.test_tier2_boundaries
   ```
   *Expected outcome*: 120 tests run, 0 failures, exit code 0.

3. **Run Native C++ Interface & Filesystem Tests**:
   ```powershell
   clang++ -std=c++17 -I psxrecomp/runtime/include tests/test_re3_textures_native.cpp psxrecomp/runtime/src/re3_textures.cpp -o build/test_re3_textures_native.exe
   .\build\test_re3_textures_native.exe
   Remove-Item build\test_re3_textures_native.exe
   ```
   *Expected outcome*: Exit code 0; all native tests pass.
