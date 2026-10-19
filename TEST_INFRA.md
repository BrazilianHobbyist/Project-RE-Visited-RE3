# E2E Test Infrastructure & Test Architecture Specification
**Project**: RE3Recomp Native HD Texture & 3D Model Pack Engine  
**Author**: E2E Test Suite Architect & Writer (`teamwork_preview_test_writer`)  
**Status**: ACTIVE  
**Last Updated**: 2026-10-18  

---

## 1. Overview & Objectives

The End-to-End (E2E) Test Suite for RE3Recomp validates the native HD texture, 3D model replacement, HD user interface, texture dumping tooling, and launcher integration subsystems. 

The testing architecture follows an **opaque-box E2E verification methodology** across 5 distinct tiers:
- **Tier 1: Feature Coverage** — Exhaustive contract testing covering all 17 features from `PROJECT.md § Feature Inventory` (at least 5 distinct test cases per feature, >= 85 tests).
- **Tier 2: Boundary & Corner Cases** — Extreme values, edge coordinates, clamping limits, format boundary checks, and empty/maximum dimensions.
- **Tier 3: Cross-Feature Interactions** — Pairwise interaction matrices verifying subsystems operating concurrently (Backgrounds + Models + UI + Dumping + Launcher Configuration).
- **Tier 4: Real-World Application Scenarios** — Realistic end-to-end player journeys and tool workflows (Camera Tour, Jill & Nemesis Combat, Menu Navigation, Dumping Session, Launcher Roundtrip).
- **Tier 5: Adversarial & Stress Testing** — Malformed inputs, corrupted PNG headers, path traversal security checks, unicode handling, and memory/cache saturation.

---

## 2. Directory Layout & Organization

The test suite and fixtures reside strictly within `tests/e2e/`:

```
c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\
├── TEST_INFRA.md                        # This document: architecture, contracts, command guide
├── TEST_READY.md                        # Test execution summary, readiness signoff & checklist
└── tests/
    └── e2e/
        ├── run_e2e_tests.py             # Master Python test runner CLI (supports tier filtering & reporting)
        ├── run_tests.ps1                # PowerShell wrapper for Windows CI/local execution
        ├── test_tier1_features.py       # Tier 1: 17 Features x >=5 tests (85+ tests)
        ├── test_tier2_boundaries.py     # Tier 2: Boundary & Corner Case verification (30+ tests)
        ├── test_tier3_interactions.py   # Tier 3: Pairwise Subsystem Interaction tests (15+ tests)
        ├── test_tier4_scenarios.py      # Tier 4: Real-World Gameplay & Tooling Scenarios (5+ scenarios)
        ├── test_tier5_adversarial.py    # Tier 5: Adversarial, Fault Injection & Stress tests (15+ tests)
        ├── harness/                     # Emulation, protocol and verification harness
        │   ├── __init__.py
        │   ├── xxhash64.py              # Pure-python deterministic xxHash64 implementation (XXH64 spec)
        │   ├── png_util.py              # Genuine PNG decoder/encoder (chunk validation: IHDR, IDAT, IEND)
        │   ├── config.py                # TOML parser/serializer for [textures] matching settings.toml
        │   ├── memory_state.py          # PSX guest memory state emulator (0x000D1F08/9/A, status, etc.)
        │   ├── res_resolver.py          # RESHDP naming convention and fallback resolver
        │   ├── cache.py                 # Multi-threaded async background pre-fetcher & LRU cache model
        │   ├── texture_pipeline.py      # 3D texture replacement, UV normalizer, aniso/mipmap filtering, fallback
        │   ├── ui_pipeline.py           # HD UI backdrop & sprite replacement pipeline
        │   ├── dumper.py                # Texture dumping engine with session deduplication & metadata
        │   ├── launcher_mock.py         # Win32 launcher settings persistence and dialog validator
        │   ├── fixtures.py              # Synthetic mock pack generator (SeamlessHD, PartialPack, CorruptPack)
        │   └── test_framework.py        # Test runner engine, ANSI coloring, JUnit XML & TAP reporter
        └── fixtures/                    # Temporary fixture sandbox generated during test execution
```

---

## 3. Feature Coverage Matrix (17 Features)

Every feature defined in `PROJECT.md § Feature Inventory` is mapped to explicit test cases in Tier 1 and beyond:

| # | Feature | Subsystem | Target Test Class | Minimum Cases |
|---|---------|-----------|-------------------|---------------|
| 1 | Configuration & Settings Parser | M1 Settings | `TestFeature01ConfigParser` | 5 |
| 2 | Stage/Room/Cut State Tracking | M2 Backgrounds | `TestFeature02StateTracking` | 5 |
| 3 | RESHDP Background Loading & File Resolution | M2 Backgrounds | `TestFeature03ReshdpResolution` | 5 |
| 4 | Asynchronous Background Pre-fetcher & LRU Cache | M2 Backgrounds | `TestFeature04AsyncPrefetchCache` | 5 |
| 5 | Background Rendering to `s_hr_fbo` | M2 Backgrounds | `TestFeature05FboComposition` | 5 |
| 6 | Foreground Occlusion Mask Alignment | M2 Backgrounds | `TestFeature06OcclusionMaskAlignment` | 5 |
| 7 | xxHash64 Texture Fingerprinting | M3 3D Models | `TestFeature07XxHash64Fingerprint` | 5 |
| 8 | HD Model Replacement Pipeline | M3 3D Models | `TestFeature08ModelReplacementPipeline` | 5 |
| 9 | UV Normalization for 3D Meshes | M3 3D Models | `TestFeature09UvNormalization` | 5 |
| 10 | Anisotropic & Trilinear Quality Filtering | M3 3D Models | `TestFeature10TextureFiltering` | 5 |
| 11 | Transparent PS1 Texture Fallback | M3 3D Models | `TestFeature11TransparentFallback` | 5 |
| 12 | HD UI 2D Screen Backdrops | M4 HD UI | `TestFeature12HdUiBackdrops` | 5 |
| 13 | HD UI Sprite & Icon Replacements | M4 HD UI | `TestFeature13HdUiSprites` | 5 |
| 14 | Automated Texture Dumping Tooling | M5 Dumping | `TestFeature14TextureDumping` | 5 |
| 15 | Launcher "Mods & Texturas HD" Dialog | M6 Launcher | `TestFeature15LauncherDialog` | 5 |
| 16 | Launcher Settings Persistence | M6 Launcher | `TestFeature16LauncherPersistence` | 5 |
| 17 | E2E Regression & Quality Hardening | M7 Integration | `TestFeature17RegressionQuality` | 5 |

**Total Tier 1 Test Cases**: >= 85 tests.

---

## 4. Test Tiers Detail

### Tier 1: Feature Coverage (85+ tests)
Validates primary happy paths, return values, state transitions, and contract compliance for all 17 features.

### Tier 2: Boundary & Corner Cases (30+ tests)
Exercises extreme values:
- Stage/Room/Cut boundary IDs (`0x00`, `0xFF`, out-of-range).
- Max anisotropic filtering clamping (`0x`, `1x`, `16x`, `32x` clamped to hardware max 16x).
- UV normalization limits (`u=0, v=0`, `u=255, v=255`, degenerate UV bounds where $u_{min} = u_{max}$).
- Empty pack names (`pack = ""`), nonexistent pack directories.
- Zero-length CLUT palettes, single-pixel textures, 4K maximum resolution ($3840 \times 2160$, $4096 \times 2048$).
- LRU cache capacity limits (budget exhaustion and orderly oldest-item eviction).

### Tier 3: Cross-Feature Interactions (15+ tests)
Validates pairwise interactions:
- Backgrounds + 3D Models: 4K background blit into `s_hr_fbo` followed by actor mesh rendering with HD textures and occlusion masks.
- Models + UI: Simultaneous rendering of 3D item models inside the HD 2D inventory screen.
- Backgrounds + Dumping: Camera cut transitions with `dump_textures = true` dumping both room backdrops and actor meshes.
- Launcher Config + Runtime: Changing settings in the Launcher, persisting to `settings.toml`, and runtime initializing the correct subsystems.
- Partial Pack + Fallback: Packs containing only backgrounds but no models, or models but no UI, verifying selective fallback.

### Tier 4: Real-World Application Scenarios (5 Scenarios)
Comprehensive end-to-end simulation journeys:
1. **Scenario 1 — Room Camera Tour**: Player traverses Stage 1 Room 0 (Uptown Warehouse) -> Room 1 (Alley) -> Stage 2 Room 0 (Downtown), changing 8 camera cuts. Verifies async pre-fetching, LRU cache hits, and FBO blit coordinates.
2. **Scenario 2 — Jill & Nemesis Combat**: Battle encounter in Downtown. 3D actor meshes (Jill, Nemesis, Rocket Launcher) are textured with 64-bit xxHash64 replacements, 16x anisotropic filtering, and PS1 fallback on secondary debris.
3. **Scenario 3 — Inventory & Status Navigation**: Opening inventory menu, switching to status screen, health transitions (Fine -> Caution -> Danger -> Poison), verifying backdrop resolution and ECG pulse overlay preservation.
4. **Scenario 4 — Full Texture Dumping Session**: Running gameplay with `dump_textures = true`. Decodes actors and rooms, generates PNGs in `dump/textures/`, verifies deduplication (no redundant files written) and metadata consistency.
5. **Scenario 5 — Launcher Configuration Roundtrip**: User configures packs and checkboxes in Win32 Launcher UI, saves to `settings.toml`, reloads runtime, and verifies that unaffected sections (`[video]`, `[audio]`) remain untouched.

### Tier 5: Adversarial & Stress Testing (15+ tests)
- Corrupted PNG files (truncated chunks, invalid CRC, invalid magic bytes `\x89PNG`).
- Path traversal injection attacks in pack names (e.g., `pack = "../../../windows"`).
- Case sensitivity resolution across Windows (NTFS) and Linux.
- Memory leak and thrashing resilience under 100 consecutive rapid room cut changes.
- Recovery from malformed or corrupted `settings.toml` syntax.

---

## 5. Authoritative Expected Output Derivation

All expected outputs in this test suite are derived from explicit mathematical specifications and interface contracts:

1. **xxHash64 Fingerprint**:
   - Follows the official xxHash64 specification (Yann Collet).
   - Tested against known reference test vectors:
     - Empty buffer: `0xEF46DB3751D8E999` (seed 0)
     - Standard test vector strings with known xxHash64 digests.
   - For composite textures: hashes the 16-byte metadata key + palette words + VRAM texels.

2. **UV Normalization**:
   - Full page replacement: $u_{norm} = (u + 0.5) / 256.0$, $v_{norm} = (v + 0.5) / 256.0$.
   - Bounded sub-rectangle: $u_{norm} = (u - u_{min}) / (u_{max} - u_{min} + 1)$.

3. **RESHDP Filename Format**:
   - `R<stage><room:02X>_<cut:02X>.png` (e.g. Stage 1, Room 0x05, Cut 0x02 -> `R105_02.png`).
   - Fallback to single-digit cut: `R105_2.png`.

4. **FBO Viewport Scaling**:
   - Scale factor $S$: canvas $1024 \times S$ by $512 \times S$.
   - Sub-rectangle: $X = x \times S, Y = y \times S, W = 320 \times S, H = 240 \times S$.

5. **PNG Verification**:
   - Genuine PNG binary structure: 8-byte signature `\x89PNG\r\n\x1a\n`, valid `IHDR`, `IDAT`, and `IEND` chunks with correct zlib compression and CRC-32 checksums.

---

## 6. Execution Commands

### Running all E2E tests:
```powershell
python tests/e2e/run_e2e_tests.py
```
or via PowerShell:
```powershell
.\tests\e2e\run_tests.ps1
```

### Running individual tiers:
```powershell
python tests/e2e/run_e2e_tests.py --tier 1
python tests/e2e/run_e2e_tests.py --tier 2
python tests/e2e/run_e2e_tests.py --tier 3
python tests/e2e/run_e2e_tests.py --tier 4
python tests/e2e/run_e2e_tests.py --tier 5
```

### Running specific feature tests:
```powershell
python tests/e2e/run_e2e_tests.py -k Feature07
```

### Verbose mode with TAP or JUnit output:
```powershell
python tests/e2e/run_e2e_tests.py -v --tap
python tests/e2e/run_e2e_tests.py -v --junit report.xml
```
