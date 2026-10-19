"""
Tier 3: Cross-Feature Pairwise Interaction Test Suite for RE3Recomp HD Texture Engine.
Tests concurrent interactions across:
  - Backgrounds + 3D Models
  - 3D Models + HD UI
  - Backgrounds + HD UI
  - 3D Models + Texture Dumping
  - Backgrounds + Texture Dumping
  - Launcher Persistence + Runtime Subsystem Activation
"""

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
    compute_texture_xxhash64,
    normalize_uv_full_page,
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


class BaseTier3Test(unittest.TestCase):
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


class TestTier3CrossFeatureInteractions(BaseTier3Test):

    # ------------------------------------------------------------------------
    # 1. Backgrounds + 3D Models Interactions
    # ------------------------------------------------------------------------
    def test_i01_background_and_actor_composition(self):
        """Verify HD background blit into s_hr_fbo followed by 3D actor mesh replacement."""
        mem = PSXGuestMemory()
        mem.set_camera(1, 0, 0)

        # 1. Background resolver
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        bg_path = resolver.resolve(*mem.get_camera())
        self.assertIsNotNone(bg_path)

        # 2. 3D Model pipeline
        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD")
        actor_info = pipe.bind_texture(JILL_HASH)
        self.assertTrue(actor_info["u_is_hd"])
        self.assertEqual(actor_info["aniso"], 16.0)

    def test_i02_background_actor_and_occlusion_mask(self):
        """Verify background blit, actor render, and foreground mask quad clipping order."""
        mem = PSXGuestMemory()
        mem.set_camera(2, 0, 0)
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        self.assertIsNotNone(resolver.resolve(*mem.get_camera()))

        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD")
        nemesis_info = pipe.bind_texture(NEMESIS_HASH)
        self.assertTrue(nemesis_info["u_is_hd"])

        # Foreground obstacle (e.g. lamppost between camera and Nemesis)
        mask = compute_occlusion_mask_alignment(100, 80, 40, 160, fbo_scale=4)
        self.assertTrue(mask["is_aligned"])
        self.assertEqual(mask["fbo_rect"], (400, 320, 160, 640))

    def test_i03_selective_disable_backgrounds_only(self):
        """When hd_backgrounds=false but hd_models=true: background falls back, models use HD."""
        resolver = ReshdpResolver(self.sandbox.mods_root, "")  # empty pack -> fallback
        bg_path = resolver.resolve(1, 0, 0)
        self.assertIsNone(bg_path)  # native PS1 background fallback

        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD", enabled=True)
        jill_info = pipe.bind_texture(JILL_HASH)
        self.assertTrue(jill_info["u_is_hd"])  # model still gets HD

    def test_i04_selective_disable_models_only(self):
        """When hd_backgrounds=true but hd_models=false: background gets HD, models fall back."""
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        bg_path = resolver.resolve(1, 0, 0)
        self.assertIsNotNone(bg_path)

        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD", enabled=False)
        jill_info = pipe.bind_texture(JILL_HASH)
        self.assertFalse(jill_info["u_is_hd"])
        self.assertTrue(jill_info["fallback"])

    # ------------------------------------------------------------------------
    # 2. 3D Models + HD UI Interactions
    # ------------------------------------------------------------------------
    def test_i05_3d_item_inspection_in_hd_inventory(self):
        """Rendering a 3D model (e.g. Weapon) inside the HD inventory backdrop."""
        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")
        inv_info = ui.load_backdrop("inventory")
        self.assertTrue(inv_info["has_hd"])

        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD")
        weapon_info = pipe.bind_texture(WEAPON_HASH)
        self.assertTrue(weapon_info["u_is_hd"])

    def test_i06_combat_damage_ecg_status_transition(self):
        """Simulate Nemesis attack causing Jill condition to drop and ECG backdrop to update."""
        mem = PSXGuestMemory()
        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")

        # Initial state: Fine
        mem.set_player_condition(CONDITION_FINE)
        bg_fine = ui.resolve_status_backdrop(mem.get_player_condition())
        self.assertIn("status_fine.png", bg_fine)

        # Jill takes heavy blow from Nemesis: Danger
        mem.set_player_condition(CONDITION_DANGER)
        bg_danger = ui.resolve_status_backdrop(mem.get_player_condition())
        self.assertIn("status_danger.png", bg_danger)

        # Poisoned
        mem.set_player_condition(CONDITION_POISON)
        bg_poison = ui.resolve_status_backdrop(mem.get_player_condition())
        self.assertIn("status_poison.png", bg_poison)

    def test_i07_ui_sprite_and_actor_simultaneous_rendering(self):
        """Quick-use herb item icon while actor model is active on screen."""
        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")
        herb_info = ui.bind_ui_sprite(UI_HERB_HASH)
        self.assertTrue(herb_info["u_is_hd"])

        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD")
        jill_info = pipe.bind_texture(JILL_HASH)
        self.assertTrue(jill_info["u_is_hd"])

    # ------------------------------------------------------------------------
    # 3. Backgrounds + HD UI Interactions
    # ------------------------------------------------------------------------
    def test_i08_opening_map_screen_from_active_room(self):
        """Opening map screen suspends room camera and draws map backdrop."""
        mem = PSXGuestMemory()
        mem.set_camera(1, 1, 0)
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        bg_path = resolver.resolve(*mem.get_camera())
        self.assertIsNotNone(bg_path)

        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")
        map_info = ui.load_backdrop("map")
        self.assertTrue(map_info["has_hd"])

    def test_i09_typewriter_save_screen_transition(self):
        """Transitioning from room gameplay to typewriter save dialog."""
        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")
        save_info = ui.load_backdrop("save")
        self.assertTrue(save_info["has_hd"])

    def test_i10_epilogue_gallery_navigation(self):
        """Navigating through epilogues 0 to 7 in secret gallery."""
        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")
        for ep in range(8):
            info = ui.load_backdrop(f"epilogue_{ep}")
            self.assertTrue(info["has_hd"])

    # ------------------------------------------------------------------------
    # 4. 3D Models + Texture Dumping Interactions
    # ------------------------------------------------------------------------
    def test_i11_simultaneous_actor_render_and_dump(self):
        """3D actors rendered in game are dumped to disk if dump_textures=true."""
        dump_dir = os.path.join(self.sandbox.root, "dump_i11")
        dumper = TextureDumper(dump_dir, enabled=True)
        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD")

        for h, tp, cl in [(JILL_HASH, JILL_TPAGE, JILL_CLUT), (NEMESIS_HASH, NEMESIS_TPAGE, NEMESIS_CLUT)]:
            # Render pass
            info = pipe.bind_texture(h)
            self.assertTrue(info["u_is_hd"])
            # Dump hook
            p = dumper.dump_texture(h, tp, cl, 512, 512)
            self.assertIsNotNone(p)
            self.assertTrue(os.path.isfile(p))

    def test_i12_dump_deduplication_during_animation_frames(self):
        """Running 60 frames of Jill running should only dump the texture once."""
        dump_dir = os.path.join(self.sandbox.root, "dump_i12")
        dumper = TextureDumper(dump_dir, enabled=True)
        for frame in range(60):
            dumper.dump_texture(JILL_HASH, JILL_TPAGE, JILL_CLUT, 512, 512)
        self.assertEqual(dumper.dump_count, 1)
        self.assertEqual(dumper.skipped_duplicates, 59)

    # ------------------------------------------------------------------------
    # 5. Backgrounds + Texture Dumping Interactions
    # ------------------------------------------------------------------------
    def test_i13_dump_unique_camera_cuts_during_tour(self):
        """Traversing 4 cuts with dumper active records 4 unique entries."""
        dump_dir = os.path.join(self.sandbox.root, "dump_i13")
        dumper = TextureDumper(dump_dir, enabled=True)
        cuts = [(1, 0, 0), (1, 0, 1), (1, 1, 0), (1, 1, 1)]
        for s, r, c in cuts:
            pseudo_hash = (s << 16) | (r << 8) | c
            dumper.dump_texture(pseudo_hash, 0, 0, 320, 240)
        self.assertEqual(dumper.dump_count, 4)

    # ------------------------------------------------------------------------
    # 6. Launcher Config + Runtime Subsystems Interactions
    # ------------------------------------------------------------------------
    def test_i14_launcher_roundtrip_updates_runtime_subsystems(self):
        """Configuring launcher persists settings and updates runtime subsystems."""
        dlg = LauncherTexturesDialogMock(self.sandbox.root, self.sandbox.settings_path)
        dlg.select_pack("ClassicHD")
        dlg.chk_hd_models = False
        dlg.chk_dump_textures = True
        dlg.save_to_settings()

        # Runtime reloads settings.toml
        mgr = SettingsTomlManager(self.sandbox.settings_path)
        cfg = mgr.load()

        # Instantiate runtime subsystems with reloaded config
        bg_resolver = ReshdpResolver(self.sandbox.mods_root, cfg.pack)
        self.assertEqual(bg_resolver.pack_name, "ClassicHD")

        pipe = HDTexturePipeline(self.sandbox.mods_root, cfg.pack, enabled=cfg.hd_models)
        info = pipe.bind_texture(JILL_HASH)
        self.assertTrue(info["fallback"])  # disabled in launcher!

        dumper = TextureDumper(self.sandbox.dump_root, enabled=cfg.dump_textures)
        self.assertTrue(dumper.enabled)

    def test_i15_partial_pack_cross_subsystem_fallback(self):
        """IncompletePack has backgrounds but no models or UI: graceful selective fallback."""
        bg_resolver = ReshdpResolver(self.sandbox.mods_root, "IncompletePack")
        bg_path = bg_resolver.resolve(1, 0, 0)
        self.assertIsNotNone(bg_path)  # BG succeeds

        pipe = HDTexturePipeline(self.sandbox.mods_root, "IncompletePack")
        model_info = pipe.bind_texture(JILL_HASH)
        self.assertTrue(model_info["fallback"])  # Models fall back

        ui = HDUIPipeline(self.sandbox.mods_root, "IncompletePack")
        ui_info = ui.load_backdrop("inventory")
        self.assertTrue(ui_info["fallback_native_ui"])  # UI falls back


if __name__ == "__main__":
    unittest.main()
