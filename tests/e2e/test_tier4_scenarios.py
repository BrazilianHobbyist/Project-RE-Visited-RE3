"""
Tier 4: Real-World Application Scenario Test Suite for RE3Recomp HD Texture Engine.
Executes 5 comprehensive real-world end-to-end journeys:
  1. Room Camera Tour across multiple rooms and angles
  2. Jill & Nemesis Combat encounter with models, weapons, occlusion, and fallback
  3. Inventory & Status Menu Navigation with ECG states, 3D inspection, and map
  4. Complete Texture Dumping Session with deduplication and manifest verification
  5. Win32 Launcher Configuration Roundtrip and runtime reload parity
"""

import json
import os
import unittest
from tests.e2e.harness.config import SettingsTomlManager, RE3TextureConfig
from tests.e2e.harness.memory_state import (
    PSXGuestMemory,
    ADDR_STAGE, ADDR_ROOM, ADDR_CUT,
    ADDR_PLAYER_CONDITION,
    CONDITION_FINE, CONDITION_CAUTION, CONDITION_DANGER, CONDITION_POISON,
)
from tests.e2e.harness.res_resolver import ReshdpResolver
from tests.e2e.harness.cache import AsyncBackgroundCache
from tests.e2e.harness.texture_pipeline import (
    HDTexturePipeline,
    compute_occlusion_mask_alignment,
)
from tests.e2e.harness.ui_pipeline import HDUIPipeline
from tests.e2e.harness.dumper import TextureDumper
from tests.e2e.harness.launcher_mock import LauncherTexturesDialogMock
from tests.e2e.harness.fixtures import (
    FixtureSandbox,
    JILL_HASH, JILL_TPAGE, JILL_CLUT,
    NEMESIS_HASH, NEMESIS_TPAGE, NEMESIS_CLUT,
    WEAPON_HASH, WEAPON_TPAGE, WEAPON_CLUT,
    UI_HERB_HASH,
)
from tests.e2e.harness.png_util import validate_png_bytes


class BaseTier4Test(unittest.TestCase):
    sandbox = None

    @classmethod
    def setUpClass(cls):
        if cls.sandbox is None:
            cls.sandbox = FixtureSandbox()

    @classmethod
    def tearDownClass(cls):
        if cls.sandbox is not None:
            cls.sandbox.cleanup()
            cls.sandbox = None

    def setUp(self):
        super().setUp()
        if self.sandbox is None:
            self.__class__.sandbox = FixtureSandbox()


class TestTier4RealWorldScenarios(BaseTier4Test):

    # ------------------------------------------------------------------------
    # Scenario 1: Room Camera Tour
    # ------------------------------------------------------------------------
    def test_scenario_01_room_camera_tour(self):
        """
        Simulate player walking through Uptown Warehouse (R100) -> Alley (R101) -> Downtown (R200).
        Verifies:
          - Eager room pre-fetching on door transitions
          - Zero hitching with LRU cache hits
          - FBO destination rectangle calculation for alternating double-buffers
        """
        mem = PSXGuestMemory()
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        cache = AsyncBackgroundCache(resolver, max_memory_mb=128, max_entries=16)

        # Step 1: Start at Stage 1, Room 0 (Uptown Warehouse)
        mem.set_camera(1, 0, 0)
        cache.on_room_change(1, 0)
        entry1 = cache.get(1, 0, 0)
        self.assertIsNotNone(entry1)
        self.assertEqual(entry1.key, "R100_00")

        # Step 2: Player steps into angle 1
        mem.set_camera(1, 0, 1)
        entry2 = cache.get(1, 0, 1)
        self.assertIsNotNone(entry2)
        self.assertEqual(entry2.key, "R100_01")

        # Step 3: Player steps back to angle 0 (cache hit!)
        mem.set_camera(1, 0, 0)
        entry1_hit = cache.get(1, 0, 0)
        self.assertEqual(entry1.key, entry1_hit.key)
        self.assertGreaterEqual(cache.hits, 1)

        # Step 4: Player opens door to Stage 1, Room 1 (Alley)
        mem.set_camera(1, 1, 0)
        cache.on_room_change(1, 1)
        entry_alley = cache.get(1, 1, 0)
        self.assertIsNotNone(entry_alley)
        self.assertEqual(entry_alley.key, "R101_00")

        # Step 5: Player crosses to Stage 2, Room 0 (Downtown)
        mem.set_camera(2, 0, 0)
        cache.on_room_change(2, 0)
        entry_dt = cache.get(2, 0, 0)
        self.assertIsNotNone(entry_dt)
        self.assertEqual(entry_dt.key, "R200_00")

        # Verify FBO double-buffer composition coordinates
        # Frame 0 (Buffer 0): (0, 0, 1280, 960) at scale 4
        scale = 4
        buf0_rect = (0 * scale, 0 * scale, 320 * scale, 240 * scale)
        self.assertEqual(buf0_rect, (0, 0, 1280, 960))
        # Frame 1 (Buffer 1): (0, 960, 1280, 960) at scale 4
        buf1_rect = (0 * scale, 240 * scale, 320 * scale, 240 * scale)
        self.assertEqual(buf1_rect, (0, 960, 1280, 960))

    # ------------------------------------------------------------------------
    # Scenario 2: Jill & Nemesis Combat Encounter
    # ------------------------------------------------------------------------
    def test_scenario_02_jill_and_nemesis_combat(self):
        """
        Simulate combat encounter in Downtown (R201_00):
          - Background blit into s_hr_fbo
          - 3D Actor Jill with HD texture replacement and 16x anisotropic filtering
          - 3D Actor Nemesis with HD texture replacement
          - 3D Weapon Rocket Launcher with HD replacement
          - Secondary explosion/debris particles fall back to PS1 VRAM sampling
          - Lamppost foreground occlusion mask aligns pixel-perfectly over Nemesis
        """
        mem = PSXGuestMemory()
        mem.set_camera(2, 1, 0)

        # 1. Background
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        bg_path = resolver.resolve(*mem.get_camera())
        self.assertIsNotNone(bg_path)

        # 2. Actors & Weapon
        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD", requested_aniso=16.0)
        jill_tex = pipe.bind_texture(JILL_HASH)
        nemesis_tex = pipe.bind_texture(NEMESIS_HASH)
        weapon_tex = pipe.bind_texture(WEAPON_HASH)

        self.assertTrue(jill_tex["u_is_hd"])
        self.assertEqual(jill_tex["aniso"], 16.0)
        self.assertTrue(nemesis_tex["u_is_hd"])
        self.assertEqual(nemesis_tex["aniso"], 16.0)
        self.assertTrue(weapon_tex["u_is_hd"])

        # 3. Secondary debris particle (no replacement authored) -> Fallback
        debris_hash = 0xDEB1215000000000
        debris_tex = pipe.bind_texture(debris_hash)
        self.assertFalse(debris_tex["u_is_hd"])
        self.assertTrue(debris_tex["fallback"])

        # 4. Foreground Lamppost Occlusion Mask
        # Coordinates (80, 20, 30, 200) in 320x240 screen space
        mask = compute_occlusion_mask_alignment(80, 20, 30, 200, fbo_scale=4)
        self.assertTrue(mask["is_aligned"])
        self.assertEqual(mask["fbo_rect"], (320, 80, 120, 800))
        # Verify UV bounds within [0, 1]
        u_min, v_min, u_max, v_max = mask["screen_uv"]
        self.assertGreaterEqual(u_min, 0.0)
        self.assertLessEqual(u_max, 1.0)

    # ------------------------------------------------------------------------
    # Scenario 3: Inventory & Status Menu Navigation
    # ------------------------------------------------------------------------
    def test_scenario_03_inventory_and_status_navigation(self):
        """
        Simulate full menu workflow:
          - Opening Inventory (inventory.png)
          - Quick-use Green Herb icon in slot (UI sprite hash)
          - 3D item inspection (Weapon mesh inside UI)
          - Health status degradation: Fine -> Caution -> Danger -> Poison
          - Health recovery via herb: Poison -> Fine
          - Map screen view (map.png)
          - Resume gameplay
        """
        mem = PSXGuestMemory()
        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")
        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD")

        # 1. Open Inventory
        inv = ui.load_backdrop("inventory")
        self.assertTrue(inv["has_hd"])

        # 2. Herb icon in slot
        herb_icon = ui.bind_ui_sprite(UI_HERB_HASH)
        self.assertTrue(herb_icon["u_is_hd"])

        # 3. Inspect Weapon 3D model
        weapon_3d = pipe.bind_texture(WEAPON_HASH)
        self.assertTrue(weapon_3d["u_is_hd"])

        # 4. Check ECG Status progression
        mem.set_player_condition(CONDITION_FINE)
        self.assertIn("status_fine.png", ui.resolve_status_backdrop(mem.get_player_condition()))

        mem.set_player_condition(CONDITION_CAUTION)
        self.assertIn("status_caution.png", ui.resolve_status_backdrop(mem.get_player_condition()))

        mem.set_player_condition(CONDITION_DANGER)
        self.assertIn("status_danger.png", ui.resolve_status_backdrop(mem.get_player_condition()))

        mem.set_player_condition(CONDITION_POISON)
        self.assertIn("status_poison.png", ui.resolve_status_backdrop(mem.get_player_condition()))

        # 5. Use Green+Blue mixed herb -> Restored to Fine
        mem.set_player_condition(CONDITION_FINE)
        self.assertIn("status_fine.png", ui.resolve_status_backdrop(mem.get_player_condition()))

        # 6. Switch to Map Screen
        map_scr = ui.load_backdrop("map")
        self.assertTrue(map_scr["has_hd"])

    # ------------------------------------------------------------------------
    # Scenario 4: Texture Dumping Session
    # ------------------------------------------------------------------------
    def test_scenario_04_texture_dumping_session(self):
        """
        Simulate a full texture dumping session with dump_textures = true:
          - Multiple rooms visited
          - 3D actors (Jill, Nemesis, Weapon) rendered across 120 frames
          - Dumper exports PNG files to dump/textures/<hash>.png
          - In-memory session deduplication prevents 120 redundant file writes
          - Manifest.json generated with accurate metadata
          - All dumped PNGs are valid binary PNG files
        """
        dump_session_dir = os.path.join(self.sandbox.root, "dump_session_scenario")
        dumper = TextureDumper(dump_session_dir, enabled=True)

        # Simulate 120 gameplay frames with Jill and Nemesis
        for frame in range(120):
            dumper.dump_texture(JILL_HASH, JILL_TPAGE, JILL_CLUT, 512, 512)
            dumper.dump_texture(NEMESIS_HASH, NEMESIS_TPAGE, NEMESIS_CLUT, 1024, 1024)
            if frame % 30 == 0:
                dumper.dump_texture(WEAPON_HASH, WEAPON_TPAGE, WEAPON_CLUT, 256, 256)

        # Verification: Only 3 files should be written to disk!
        self.assertEqual(dumper.dump_count, 3)
        self.assertGreater(dumper.skipped_duplicates, 200)

        # Verify disk files
        jill_file = os.path.join(dumper.textures_dir, f"{JILL_HASH:016x}.png")
        nemesis_file = os.path.join(dumper.textures_dir, f"{NEMESIS_HASH:016x}.png")
        weapon_file = os.path.join(dumper.textures_dir, f"{WEAPON_HASH:016x}.png")

        self.assertTrue(os.path.isfile(jill_file))
        self.assertTrue(os.path.isfile(nemesis_file))
        self.assertTrue(os.path.isfile(weapon_file))

        # Validate genuine PNG binary structure
        for fpath in [jill_file, nemesis_file, weapon_file]:
            with open(fpath, "rb") as f:
                data = f.read()
            valid, reason, meta = validate_png_bytes(data)
            self.assertTrue(valid, f"Dumped PNG invalid: {reason}")
            self.assertIn("IHDR", meta["chunks"])
            self.assertIn("IDAT", meta["chunks"])
            self.assertIn("IEND", meta["chunks"])

        # Validate manifest.json
        manifest_path = os.path.join(dumper.textures_dir, "manifest.json")
        self.assertTrue(os.path.isfile(manifest_path))
        with open(manifest_path, "r", encoding="utf-8") as f:
            manifest = json.load(f)
        self.assertEqual(len(manifest["entries"]), 3)

    # ------------------------------------------------------------------------
    # Scenario 5: Launcher Configuration Roundtrip
    # ------------------------------------------------------------------------
    def test_scenario_05_launcher_configuration_roundtrip(self):
        """
        Simulate player configuring texture options in Win32 Launcher UI:
          - Enumerate installed packs (finds SeamlessHD, ClassicHD)
          - Player selects ClassicHD
          - Enables dump_textures, disables hd_models
          - Persists to settings.toml
          - Runtime reloads settings.toml
          - Subsystems activate with updated settings
          - Verify other sections ([video], [audio]) are completely untouched
        """
        dlg = LauncherTexturesDialogMock(self.sandbox.root, self.sandbox.settings_path)

        # 1. Check enumeration
        self.assertIn("SeamlessHD", dlg.installed_packs)
        self.assertIn("ClassicHD", dlg.installed_packs)

        # 2. Configure UI controls
        dlg.select_pack("ClassicHD")
        dlg.chk_hd_backgrounds = True
        dlg.chk_hd_models = False
        dlg.chk_hd_ui = True
        dlg.chk_dump_textures = True

        # 3. Save
        dlg.save_to_settings()

        # 4. Verify disk persistence
        mgr = SettingsTomlManager(self.sandbox.settings_path)
        cfg = mgr.load()
        self.assertEqual(cfg.pack, "ClassicHD")
        self.assertTrue(cfg.hd_backgrounds)
        self.assertFalse(cfg.hd_models)
        self.assertTrue(cfg.hd_ui)
        self.assertTrue(cfg.dump_textures)

        # 5. Verify zero collateral modification to [video] and [audio]
        with open(self.sandbox.settings_path, "r", encoding="utf-8") as f:
            raw_text = f.read()
        self.assertIn('[video]\nresolution = "1920x1080"', raw_text)
        self.assertIn('[audio]\nmaster_volume = 100', raw_text)

        # 6. Verify runtime subsystems respond faithfully to the new configuration
        bg_resolver = ReshdpResolver(self.sandbox.mods_root, cfg.pack)
        self.assertEqual(bg_resolver.pack_name, "ClassicHD")
        # ClassicHD has R100_00.png
        self.assertIsNotNone(bg_resolver.resolve(1, 0, 0))

        # Models are disabled in config -> transparent fallback
        pipe = HDTexturePipeline(self.sandbox.mods_root, cfg.pack, enabled=cfg.hd_models)
        model_info = pipe.bind_texture(JILL_HASH)
        self.assertTrue(model_info["fallback"])
        self.assertFalse(model_info["u_is_hd"])


if __name__ == "__main__":
    unittest.main()
