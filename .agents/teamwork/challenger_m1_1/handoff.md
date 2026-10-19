# Handoff Report: Milestone 1 Adversarial Stress-Testing (Settings & Config Engine)

**Agent**: Challenger 1 (`challenger_m1_1`)  
**Role**: Empirical Challenger / Critic / Specialist  
**Working Directory**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m1_1\`  
**Target Milestone**: M1 (Settings & Config Engine)  
**Date**: 2026-10-18  
**Verdict**: **REQUEST_CHANGES**

---

## 1. Observation

1. **C++ Runtime Parser Implementation in `main.cpp`**:
   - In `psxrecomp/runtime/src/main.cpp` (lines 5706-5714):
     ```cpp
     auto trim = [](std::string& s) {
         size_t p1 = s.find_first_not_of(" \t\r\n\"");
         size_t p2 = s.find_last_not_of(" \t\r\n\"");
         if (p1 == std::string::npos) s.clear();
         else s = s.substr(p1, p2 - p1 + 1);
     };
     trim(key);
     trim(val);
     for (char& c : key) c = (char)std::tolower((unsigned char)c);
     ```
   - In lines 5766-5777:
     ```cpp
     else if (key == "hd_backgrounds") {
         g_re3_tex_config.hd_backgrounds = (val == "true" || val == "1");
     }
     else if (key == "hd_models" || key == "hd_textures") {
         g_re3_tex_config.hd_models = (val == "true" || val == "1");
     }
     else if (key == "hd_ui" || key == "hd_menus") {
         g_re3_tex_config.hd_ui = (val == "true" || val == "1");
     }
     else if (key == "dump_textures") {
         g_re3_tex_config.dump_textures = (val == "true" || val == "1");
     }
     ```
   - In `psxrecomp/runtime/src/re3_textures.cpp` (lines 48-52):
     ```cpp
     std::string pack_name = g_re3_tex_config.pack;
     size_t p1 = pack_name.find_first_not_of(" \t\r\n\"");
     size_t p2 = pack_name.find_last_not_of(" \t\r\n\"");
     if (p1 == std::string::npos) pack_name.clear();
     else pack_name = pack_name.substr(p1, p2 - p1 + 1);
     ```

2. **Empirical Execution of Standalone C++ Test Harness (`tests/test_settings_parser_adversarial.cpp`)**:
   - Compiling and executing via:
     `clang++ -std=c++17 -I psxrecomp/runtime/include tests/test_settings_parser_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp -o build/test_settings_parser_adversarial.exe`
     `.\build\test_settings_parser_adversarial.exe`
   - Test harness output verbatim:
     ```
     =================================================================
     RE3Recomp M1 Adversarial Stress Test Harness (Settings & Config)
     =================================================================
     [TEST] 1. Missing [textures] section
       [PASS] Sensible defaults preserved when [textures] is omitted
     [TEST] 2. Quote variations
       [PASS] Double quotes: pack = "CustomPackHD" -> "CustomPackHD"
       [PASS] Unquoted: pack = CustomPackHD -> "CustomPackHD"
       [BUG CONFIRMED] Single quotes NOT stripped: pack = 'CustomPackHD' -> "'CustomPackHD'"
     [TEST] 3. Spaces around '=' and extra whitespace
       [PASS] Handles irregular spaces and tabs around '=' and values
     [TEST] 4. Case-insensitivity of boolean values
       [PASS] dump_textures = true  -> parsed as TRUE
       [BUG CONFIRMED] dump_textures = True  -> parsed as FALSE! (val is case-sensitive)
       [BUG CONFIRMED] dump_textures = TRUE  -> parsed as FALSE! (val is case-sensitive)
       [PASS] dump_textures = 1     -> parsed as TRUE
       [PASS] hd_backgrounds = false -> parsed as FALSE
       [PASS] hd_backgrounds = False -> parsed as FALSE
       [PASS] hd_backgrounds = FALSE -> parsed as FALSE
       [PASS] hd_backgrounds = 0     -> parsed as FALSE
     [TEST] 5. Empty pack string, commented out lines, and inline comments
       [PASS] Full line comments are correctly skipped
       [PASS] Explicit empty string pack = "" gives empty pack
       [PASS] Empty value pack = gives empty pack
       [BUG CONFIRMED] Inline comment corrupted string: "SeamlessHD" # Active pack"
       [BUG CONFIRMED] Inline comment broke boolean: hd_backgrounds evaluated to FALSE!
     [TEST] 6. Random/unrecognized keys inside [textures]
       [PASS] Unknown keys and malformed lines ignored gracefully without crash or corruption
     [TEST] 7. Lifecycle integration with re3_textures_init
     psxrecomp: [Textures] Active pack: "SeamlessHD" at "C:/Users/victo/AppData/Local/Temp/re3_adversarial_test_env/mods/textures/SeamlessHD" [BG:ON, Models:ON, UI:ON, Dump:OFF]
       [PASS] Standard pack SeamlessHD is valid
     psxrecomp: [Textures] Subsystem shutdown complete.
     psxrecomp: [Textures] Pack folder not found at "C:/Users/victo/AppData/Local/Temp/re3_adversarial_test_env/mods/textures/'SeamlessHD'". Vanilla textures active.
       [IMPACT CONFIRMED] Leaked single quotes invalidate pack (folder 'SeamlessHD' not found)!
     psxrecomp: [Textures] Subsystem shutdown complete.
     =================================================================
     Adversarial test harness execution completed.
     =================================================================
     ```

3. **Discrepancy with Python E2E Harness (`tests/e2e/harness/config.py`)**:
   - `tests/e2e/harness/config.py` converts boolean values to lowercase (`v_low = val.strip().lower()`), strips single quotes (`elif pack_val.startswith("'") and pack_val.endswith("'"): pack_val = pack_val[1:-1]`), and strips inline comments (`v = v.split("#", 1)[0].strip()`).
   - Running `python -m unittest tests.test_settings_parser_adversarial` passes 3/3 tests in Python.
   - This masked the failure in the C++ runtime parser from the existing automated Python E2E tests.

---

## 2. Logic Chain

1. **Boolean Case-Insensitivity Failure**:
   - Observation 1 demonstrates that in `main.cpp`, `key` is converted to lowercase via `for (char& c : key) c = (char)std::tolower(...)`, but `val` is left unmodified.
   - When boolean fields like `dump_textures` or `hd_backgrounds` are checked via `(val == "true" || val == "1")`, any capitalized variant (such as `True` or `TRUE`, standard in Python/TOML generators or user edits) fails the equality check.
   - As observed empirically in Observation 2, `dump_textures = True` evaluates to `false`, silently disabling feature activation without warning.

2. **Single Quote Retention and Path Corruption**:
   - Observation 1 demonstrates that `trim()` in `main.cpp` and `re3_textures.cpp` only searches for `" \t\r\n\"`. Single quote `'` is not included in the trim character set.
   - In TOML, single quotes denote literal strings: `pack = 'SeamlessHD'`.
   - When parsed, `val` retains the enclosing `'` characters (e.g. `'CustomPackHD'`).
   - In `re3_textures_init()`, filesystem lookup constructs `mods/textures/'CustomPackHD'`, which fails `std::filesystem::exists()` on Windows, causing valid texture packs to be rejected and falling back to vanilla PS1 textures (Observation 2, Test 7).

3. **Inline Comment Handling**:
   - In `main.cpp`, lines are split only on `=`. If a user appends an inline comment (e.g. `hd_backgrounds = true # Enable backgrounds`), `val` contains `true # Enable backgrounds`.
   - `val == "true"` evaluates to `false`, disabling HD backgrounds.
   - Similarly, string pack names append `# ...` into the pack path.

4. **Section Scoping**:
   - In `main.cpp:5701`, `if (line[first] == '[') continue;` ignores section headers without recording section state. A key named `pack` in another section would overwrite `g_re3_tex_config.pack`.

---

## 3. Caveats

- The C++ runtime handles missing sections, arbitrary whitespace around `=`, unquoted pack strings, double-quoted pack strings, fully commented lines (`# ...`), empty pack values, and unrecognized keys gracefully without crashing or buffer overflows.
- Memory safety of string copy is guarded by `sizeof(...) - 1` with NUL termination.

---

## 4. Conclusion

Verdict: **REQUEST_CHANGES**.

While the core architecture and basic happy path function properly, the runtime parser in `psxrecomp/runtime/src/main.cpp` fails adversarial stress-testing in three concrete areas specified by the Milestone 1 requirements:
1. **Case-insensitivity**: `val` is not lowercased, causing `True`/`TRUE` to evaluate to `false`.
2. **Quote stripping**: Single quotes (`'`) are not trimmed from strings, breaking folder path resolution for `pack = 'SeamlessHD'`.
3. **Inline comments**: Unstripped inline comments after values corrupt values and break boolean flags.

### Concrete Recommendations for Worker / Fix:
1. In `main.cpp` inside `load_re3_qol_settings()`:
   - Strip inline comments from `line` before or after splitting on `=`:
     ```cpp
     size_t comment_pos = val.find('#');
     if (comment_pos != std::string::npos) val = val.substr(0, comment_pos);
     ```
   - Update `trim()` to strip `'` as well as `"`:
     ```cpp
     size_t p1 = s.find_first_not_of(" \t\r\n\"'");
     size_t p2 = s.find_last_not_of(" \t\r\n\"'");
     ```
   - Also update `re3_textures.cpp` line 49-50 to include `'` in the trim set.
   - Lowercase `val` for boolean parsing or provide a case-insensitive helper:
     ```cpp
     std::string val_lower = val;
     for (char& c : val_lower) c = (char)std::tolower((unsigned char)c);
     bool is_true = (val_lower == "true" || val_lower == "1" || val_lower == "yes" || val_lower == "on");
     ```
   - Track section state (`current_section`) so keys outside `[textures]` do not collide.

---

## 5. Verification Method

To independently verify these findings:

1. **Compile and Run Standalone C++ Adversarial Test Harness**:
   ```powershell
   clang++ -std=c++17 -I psxrecomp/runtime/include tests/test_settings_parser_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp -o build/test_settings_parser_adversarial.exe
   .\build\test_settings_parser_adversarial.exe
   ```
   *Expected Output*: Displays `[BUG CONFIRMED]` for single quotes, `True`/`TRUE`, and inline comments.

2. **Run Python E2E Comparison Test**:
   ```powershell
   python -m unittest tests.test_settings_parser_adversarial
   ```
   *Expected Output*: Passes in Python mock, highlighting the contract discrepancy with C++ runtime.
