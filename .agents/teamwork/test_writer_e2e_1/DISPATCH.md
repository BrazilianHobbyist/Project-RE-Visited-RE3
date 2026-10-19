## 2026-10-18T03:39:48Z
You are E2E Test Suite Architect & Writer (teamwork_preview_test_writer).
Your working directory is: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\test_writer_e2e_1\
Project root: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp
Original request file: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\.agents\teamwork\ORIGINAL_REQUEST.md
Project plan file: c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\PROJECT.md

MANDATORY FIRST STEP:
Read ORIGINAL_REQUEST.md and PROJECT.md.

YOUR MISSION:
Design, build, and verify a comprehensive, opaque-box E2E test suite for the RE3Recomp HD Texture & 3D Model Pack Engine, covering all 17 features from PROJECT.md § Feature Inventory.

Test Design Methodology:
- Tier 1: Feature Coverage (>=5 test cases per feature for all 17 features = at least 85 test cases).
- Tier 2: Boundary & Corner Cases (>=5 per feature where boundaries exist).
- Tier 3: Cross-Feature Interactions (pairwise combinations: backgrounds + models + UI + dumping + launcher config).
- Tier 4: Real-World Application Scenarios (>= 5 realistic scenarios: room camera tour, Jill & Nemesis combat, inventory/status menu navigation, texture dumping session, launcher config roundtrip).

Deliverables:
1. Create `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\TEST_INFRA.md` following the template in Project Pattern instructions.
2. Implement the test suite and harness in `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\tests\e2e\` (e.g. Python or PowerShell automated test runner `run_e2e_tests.py` or `run_tests.ps1`, mock texture packs, RESHDP samples, hash verification tests, config fixtures).
3. Execute the test runner (it should be able to run and report status cleanly).
4. Publish `c:\Users\victo\Documents\Projetos\Decomp - SOTC\RE3Recomp\TEST_READY.md` at project root with test commands, coverage summary, and feature checklist.
5. Write handoff.md in your working directory and notify the orchestrator via send_message.

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All test implementations and fixtures must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.
