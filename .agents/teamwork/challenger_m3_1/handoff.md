# Handoff Report: Challenger M3-1 (xxHash64 Adversarial & Empirical Verification)

**Role**: Challenger M3-1 (`teamwork_preview_challenger`)  
**Archetype**: EMPIRICAL CHALLENGER (critic, specialist)  
**Parent ID**: `7e5b962c-58f6-46df-a295-efcce3929cc7`  
**Working Directory**: `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\challenger_m3_1\`  
**Milestone**: M3-1 (3D Model & Actor HD Texture Subsystem — xxHash64 Fingerprinting)  
**Empirical Verdict**: **CONFIRMED**

---

## 1. Observation

Direct, empirical observations obtained by authoring, compiling, and running specialized adversarial test harnesses:

1. **Bit-Exact Golden Reference Parity**:
   - `psxrecomp/runtime/include/xxhash.h` (`XXH_INLINE_ALL`) and `psxrecomp/runtime/src/re3_hd_textures.cpp`:
     - Jill (`tpage=0x0005`, `clut=0x7C00`, `lim=(0, 0, 255, 255)`): C++ output `0x2a8d3a5fc58a1180ULL` bit-exactly matches Python fixture `JILL_HASH` (`3066171104319639936`).
     - Nemesis (`tpage=0x0006`, `clut=0x7C10`, `lim=(0, 0, 255, 255)`): C++ output `0x5eb27155676517dbULL` bit-exactly matches Python fixture `NEMESIS_HASH` (`6823640997040560091`).
     - Weapon (`tpage=0x0007`, `clut=0x7C20`, `lim=(0, 0, 127, 127)`): C++ output `0x80c806206d596532ULL` bit-exactly matches Python fixture `WEAPON_HASH` (`9279673768540202290`).
     - UI Herb (`tpage=0x0008`, `clut=0x7C30`, `lim=(0, 0, 31, 31)`): C++ output `0x2662656daac1131bULL` bit-exactly matches Python fixture `UI_HERB_HASH` (`2765884642849526555`).

2. **Extreme UV Coordinate Boundary Testing**:
   - `(0, 0, 0, 0)`: Single-pixel bounding box at origin produces deterministic non-zero digest (`0x0005`/`0x7C00` -> `0xe655ffecdd0808a2ULL`).
   - `(255, 255, 255, 255)`: Single-pixel bounding box at maximum PS1 page coordinate produces distinct non-zero digest (`0x8797b57b771340b0ULL`).
   - Inverted bounds `(255, 255, 0, 0)`: `row_words` computes to negative/zero; `re3_hd_tex_compute_from_vram` safely executes without looping or memory violation (`0xa3fca0ec0d2688ecULL`).
   - `lim = NULL`: Bit-exactly defaults to `(0, 0, 255, 255)` in both C++ and Python.
   - Out-of-bounds coordinates (e.g., `{-1000, -500, 9999, 8888}`): `re3_hd_tex_compute_from_vram` clamps coordinates via `std::clamp(coord, 0, 255)` (lines 309–312) to `[0, 0, 255, 255]`, producing identical hash to explicit full page.

3. **Depth Mode Differentiation (4bpp / 8bpp / 15bpp / invalid depth 3)**:
   - 4bpp (`depth = 0`, `tpage = 0x0005`): Hashing sensitive to 16 palette entries; mutating palette modifies digest.
   - 8bpp (`depth = 1`, `tpage = 0x0085`): Hashing sensitive to 256 palette entries; mutating palette modifies digest.
   - 15bpp direct (`depth = 2`, `tpage = 0x0105`): Palette is skipped (`depth < 2` guard at line 261 / line 320); mutating palette leaves digest invariant.
   - Invalid depth 3 (`depth = 3`, `tpage = 0x0185`): Safely executes without crash, palette ignored like direct mode.

4. **Palette Zero-Padding Equivalence & Truncation**:
   - Short palette of 2 words in 4bpp mode zero-padded up to 16 entries produces bit-exactly the same digest as a 16-word array with 14 trailing zeros (`0x23f815049c6934c2ULL == 0x23f815049c6934c2ULL`).
   - Short palette of 5 words in 8bpp mode zero-padded up to 256 entries produces bit-exactly the same digest as a 256-word array with 251 trailing zeros.
   - `palette = NULL` or `palette_len = 0` produces bit-exactly the same digest as an array filled with zeros.
   - Oversized palette (300 words passed to 4bpp mode) strictly clamps to 16 words, ignoring trailing 284 words.

5. **VRAM Circular Row Wrapping & Buffer Protection**:
   - Flanked a 1024x512 mock VRAM buffer with 4,096-word guard canary zones (`0xBEEF`) pre and post buffer.
   - Tested horizontal wrap: `tpage = 0x010F` (`base_x = 960`), 15bpp direct (`row_words = 256`). Row span `960..1215` split into `first_chunk = 64` (words 960..1023) and `second_chunk = 192` (words 0..191) at lines 346–350 of `re3_hd_textures.cpp`.
   - Tested vertical bounds: `base_y = 256`, `v_max = 255` -> `y = (256 + 255) & 511 = 511` (last row).
   - Tested CLUT wrap: `clut_x = 1008` with 256 palette entries wrapping around row 496.
   - Post-test canary inspection: 100% of canary words remained `0xBEEF` (zero bytes out of bounds, zero memory corruption).
   - Tested `vram = nullptr`: safely returns 0 without dereferencing.

6. **Collision Resistance (10,000 Payloads) & Avalanche**:
   - Generated 10,000 distinct payloads across varying `tpage`, `clut`, UV bounds, palettes, and texel configurations.
   - Both C++ (`tests/test_re3_m3_adversarial.cpp`) and Python (`tests/test_re3_m3_adversarial.py`) inserted all 10,000 digests into hash sets:
     - Total unique hashes: **10,000 / 10,000**.
     - Collisions detected: **0**.
   - Avalanche test: 1,000 single-bit mutations in CLUT, texels, and indexed palettes changed the xxHash64 digest in 100% of cases.

7. **Cross-Language Bit-Exact Parity (500 Complex Vectors)**:
   - Generated 500 pseudo-random payloads with varying styles (inverted bounds, single pixels, sub-rects, short palettes, oversized palettes, irregular texel lengths) via C++ binary `build/test_re3_m3_adversarial.exe --export-parity build/parity_vectors.bin`.
   - Executed Python validator against `build/parity_vectors.bin`:
     - Result: **500 / 500 vectors matched bit-for-bit between C++ and Python**.

8. **Existing Test Suite Integrity**:
   - `build/test_re3_hd_textures_native.exe`: 5 executed, 5 passed, 0 failed.
   - `python tests/e2e/run_e2e_tests.py`: 155 executed, 155 passed, 0 failed.

---

## 2. Logic Chain

1. **Requirement Verification**:
   - The user dispatch instructed to stress-test xxHash64 fingerprinting with boundary conditions, depth modes (4bpp/8bpp/15bpp), palette padding, VRAM row wrapping, collision testing, and C++/Python parity.
   - *Observation 1* establishes that the core xxHash64 implementation reproduces the golden fixtures of Jill, Nemesis, Weapon, and UI Herb bit-for-bit.
   - *Observation 2* and *Observation 3* prove that extreme UV limits, inverted coordinates, and all depth modes are handled deterministically without crashes or infinite loops.
   - *Observation 4* proves that palette zero-padding guarantees that truncated palettes in memory match their canonical zero-padded forms, preventing false cache misses for actors sharing partial CLUTs.
   - *Observation 5* proves that circular VRAM addressing `(x & 1023)` and row splitting `(1024 - x)` prevent heap/stack buffer overruns when primitives sample near the edge of VRAM. Canary zones confirm zero memory corruption.
   - *Observation 6* proves empirical collision resistance across 10,000 distinct textures and confirms strong avalanche behavior.
   - *Observation 7* confirms bit-exact parity across 500 randomly sampled payloads between the compiled C++ engine and the Python E2E tooling.
   - *Observation 8* confirms that no regressions were introduced into existing native or E2E tests.

---

## 3. Caveats

- **Live GPU Draw Calls**: The adversarial tests focus strictly on the computational fingerprinting engine (`re3_hd_tex_compute_hash`, `re3_hd_tex_compute_from_vram`, UV normalization, and cache keys). Live OpenGL window texture sampling during interactive gameplay relies on the SDL2 OpenGL context initialized during the full game execution.
- No caveats regarding Milestone 3 xxHash64 fingerprinting: all boundary conditions, depth modes, wrapping modes, and parity criteria are fully satisfied.

---

## 4. Conclusion

**Verdict: CONFIRMED**

The xxHash64 texture fingerprinting subsystem is robust, memory-safe, mathematically sound, collision-resistant, and bit-for-bit identical between C++ and Python. No defects were found.

---

## 5. Verification Method

To independently reproduce the adversarial verification results:

1. **Run Native C++ Adversarial Test Suite**:
   ```powershell
   clang++ -std=c++17 -DPSX_SDL3=1 -Ipsxrecomp/runtime/include -isystem "C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/include" tests/test_re3_m3_adversarial.cpp psxrecomp/runtime/src/re3_textures.cpp psxrecomp/runtime/src/re3_hd_textures.cpp psxrecomp/runtime/src/re3_hd_backgrounds.cpp psxrecomp/runtime/src/psx_window_icon.cpp -L"C:/Users/victo/.local/share/retcomm/toolchains/cmake-clang-v1/latest/deps/lib" -lSDL3 -lwinmm -lole32 -loleaut32 -limm32 -lversion -lsetupapi -lgdi32 -luuid -lshell32 -lopengl32 -lpsapi -o build/test_re3_m3_adversarial.exe
   .\build\test_re3_m3_adversarial.exe
   ```
   *Expected Output*: `Adversarial Results: 8 executed, 8 passed, 0 failed`.

2. **Generate and Verify Cross-Language Parity (500 Vectors)**:
   ```powershell
   .\build\test_re3_m3_adversarial.exe --export-parity build/parity_vectors.bin
   python -m unittest tests/test_re3_m3_adversarial.py
   ```
   *Expected Output*: `OK`, `[PARITY CONFIRMED] 500/500 vectors matched bit-for-bit between C++ and Python.`

3. **Run Native Unit Tests**:
   ```powershell
   .\build\test_re3_hd_textures_native.exe
   ```
   *Expected Output*: `Results: 5 executed, 5 passed, 0 failed`.

4. **Run Python E2E Regression Suite**:
   ```powershell
   python tests/e2e/run_e2e_tests.py
   ```
   *Expected Output*: `Total Tests: 155, Passed: 155, Failed: 0, Errors: 0, Skipped: 0`.
