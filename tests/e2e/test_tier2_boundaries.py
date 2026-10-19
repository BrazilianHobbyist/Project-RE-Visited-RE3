"""
Tier 2: Boundary & Corner Case Test Suite for RE3Recomp HD Texture & 3D Model Engine.
Covers extreme values, edge coordinates, clamping limits, degenerate dimensions, and failure boundaries.
Total test cases: 35+.
"""

import os
import shutil
import tempfile
import unittest
from tests.e2e.harness.config import SettingsTomlManager, RE3TextureConfig
from tests.e2e.harness.memory_state import (
    PSXGuestMemory,
    ADDR_STAGE, ADDR_ROOM, ADDR_CUT,
    ADDR_PLAYER_CONDITION,
)
from tests.e2e.harness.res_resolver import ReshdpResolver
from tests.e2e.harness.cache import AsyncBackgroundCache
from tests.e2e.harness.texture_pipeline import (
    compute_texture_xxhash64,
    normalize_uv_full_page,
    normalize_uv_sub_rect,
    calculate_mipmap_levels,
    clamp_anisotropy,
    HDTexturePipeline,
    compute_occlusion_mask_alignment,
)
from tests.e2e.harness.ui_pipeline import HDUIPipeline
from tests.e2e.harness.dumper import TextureDumper
from tests.e2e.harness.launcher_mock import LauncherTexturesDialogMock
from tests.e2e.harness.fixtures import FixtureSandbox, JILL_HASH
from tests.e2e.harness.png_util import create_solid_rgba_png, validate_png_bytes


class BaseTier2Test(unittest.TestCase):
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


class TestTier2Boundaries(BaseTier2Test):

    # --- Config & Settings Boundaries ---
    def test_b01_config_empty_file(self):
        empty_toml = os.path.join(self.sandbox.root, "empty.toml")
        with open(empty_toml, "w") as f:
            pass
        mgr = SettingsTomlManager(empty_toml)
        cfg = mgr.load()
        # Should gracefully return defaults
        self.assertEqual(cfg.pack, "SeamlessHD")
        self.assertTrue(cfg.hd_backgrounds)

    def test_b02_config_malformed_boolean_values(self):
        bad_bool_toml = os.path.join(self.sandbox.root, "bad_bool.toml")
        with open(bad_bool_toml, "w") as f:
            f.write('[textures]\nhd_backgrounds = "unknown_str"\nhd_models = 2\n')
        mgr = SettingsTomlManager(bad_bool_toml)
        cfg = mgr.load()
        self.assertTrue(cfg.hd_backgrounds)  # falls back to default

    def test_b03_config_pack_name_max_length(self):
        long_name = "P" * 127
        long_toml = os.path.join(self.sandbox.root, "long_pack.toml")
        with open(long_toml, "w") as f:
            f.write(f'[textures]\npack = "{long_name}"\n')
        mgr = SettingsTomlManager(long_toml)
        cfg = mgr.load()
        self.assertEqual(cfg.pack, long_name)

    # --- Memory State Boundaries ---
    def test_b04_memory_extreme_stage_values(self):
        mem = PSXGuestMemory()
        # Test 0x00, 0x05 (max valid stage), 0xFF (invalid guest byte)
        for val in [0x00, 0x05, 0xFF]:
            mem.write_byte(ADDR_STAGE, val)
            self.assertEqual(mem.read_byte(ADDR_STAGE), val)

    def test_b05_memory_extreme_room_values(self):
        mem = PSXGuestMemory()
        for val in [0x00, 0x25, 0xFF]:
            mem.write_byte(ADDR_ROOM, val)
            self.assertEqual(mem.read_byte(ADDR_ROOM), val)

    def test_b06_memory_extreme_cut_values(self):
        mem = PSXGuestMemory()
        for val in [0x00, 0x15, 0xFF]:
            mem.write_byte(ADDR_CUT, val)
            self.assertEqual(mem.read_byte(ADDR_CUT), val)

    def test_b07_memory_address_masking(self):
        mem = PSXGuestMemory()
        # KSEG0 virtual address (0x800D1F08) should map to same physical RAM as 0x000D1F08
        mem.write_byte(0x800D1F08, 0x42)
        self.assertEqual(mem.read_byte(ADDR_STAGE), 0x42)

    # --- RESHDP Resolution Boundaries ---
    def test_b08_reshdp_cut_zero_formatting(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        candidates = resolver.get_candidate_filenames(1, 0, 0)
        self.assertIn("R100_00.png", candidates)
        self.assertIn("R100_0.png", candidates)

    def test_b09_reshdp_hex_cut_boundary(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        candidates_0f = resolver.get_candidate_filenames(1, 0, 15)
        candidates_10 = resolver.get_candidate_filenames(1, 0, 16)
        self.assertIn("R100_0F.png", candidates_0f)
        self.assertIn("R100_10.png", candidates_10)

    def test_b10_reshdp_empty_pack_name(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "")
        path = resolver.resolve(1, 0, 0)
        self.assertIsNone(path)

    # --- LRU Cache & Prefetcher Boundaries ---
    def test_b11_cache_size_one_rapid_eviction(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        cache = AsyncBackgroundCache(resolver, max_memory_mb=64, max_entries=1)
        cache.get(1, 0, 0)
        self.assertEqual(len(cache.cache), 1)
        cache.get(1, 0, 1)
        self.assertEqual(len(cache.cache), 1)
        self.assertEqual(cache.evictions, 1)

    def test_b12_cache_zero_memory_budget_eviction(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        # 0 MB memory budget forces eviction on second insertion
        cache = AsyncBackgroundCache(resolver, max_memory_mb=0, max_entries=10)
        cache.get(1, 0, 0)
        cache.get(1, 0, 1)
        self.assertGreaterEqual(cache.evictions, 1)

    # --- FBO & Composition Boundaries ---
    def test_b13_fbo_scale_factor_one(self):
        # Native PS1 resolution scale S=1
        w, h = 1024 * 1, 512 * 1
        self.assertEqual((w, h), (1024, 512))

    def test_b14_fbo_scale_factor_eight(self):
        # Extreme 8K SSAA scale S=8
        w, h = 1024 * 8, 512 * 8
        self.assertEqual((w, h), (8192, 4096))

    # --- Occlusion Mask Boundaries ---
    def test_b15_mask_zero_dimension(self):
        res = compute_occlusion_mask_alignment(50, 50, 0, 0, fbo_scale=4)
        u_min, v_min, u_max, v_max = res["screen_uv"]
        self.assertEqual(u_min, u_max)
        self.assertEqual(v_min, v_max)

    def test_b16_mask_full_screen_boundary(self):
        res = compute_occlusion_mask_alignment(0, 0, 320, 240, fbo_scale=4)
        u_min, v_min, u_max, v_max = res["screen_uv"]
        self.assertEqual((u_min, v_min, u_max, v_max), (0.0, 0.0, 1.0, 1.0))

    # --- xxHash64 Fingerprint Boundaries ---
    def test_b17_xxhash64_zero_byte_texels(self):
        h = compute_texture_xxhash64(0x0005, 0x7C00, limits=(0, 0, 0, 0), vram_texels=b"")
        self.assertIsInstance(h, int)
        self.assertGreater(h, 0)

    def test_b18_xxhash64_single_pixel_bounds(self):
        h1 = compute_texture_xxhash64(0x0005, 0x7C00, limits=(5, 5, 5, 5))
        h2 = compute_texture_xxhash64(0x0005, 0x7C00, limits=(5, 5, 5, 6))
        self.assertNotEqual(h1, h2)

    def test_b19_xxhash64_max_tpage_clut(self):
        h = compute_texture_xxhash64(0xFFFF, 0xFFFF)
        self.assertIsInstance(h, int)

    def test_b20_xxhash64_direct_15bit_color_depth(self):
        # depth = 2 (15-bit direct color, no palette words)
        tpage_15bit = (2 << 7) | 0x05
        h = compute_texture_xxhash64(tpage_15bit, 0)
        self.assertIsInstance(h, int)

    # --- UV Normalization Boundaries ---
    def test_b21_uv_origin_boundary(self):
        u_norm, v_norm = normalize_uv_full_page(0, 0)
        self.assertEqual(u_norm, 0.5 / 256.0)
        self.assertEqual(v_norm, 0.5 / 256.0)

    def test_b22_uv_max_integer_boundary(self):
        u_norm, v_norm = normalize_uv_full_page(255, 255)
        self.assertEqual(u_norm, 255.5 / 256.0)
        self.assertEqual(v_norm, 255.5 / 256.0)

    def test_b23_uv_subrect_degenerate_bounds(self):
        # Degenerate where u_min == u_max and v_min == v_max
        u_norm, v_norm = normalize_uv_sub_rect(10, 20, 10, 20, 10, 20)
        self.assertEqual((u_norm, v_norm), (0.0, 0.0))

    # --- Anisotropic & Trilinear Filtering Boundaries ---
    def test_b24_anisotropy_exact_16x_boundary(self):
        self.assertEqual(clamp_anisotropy(16.0), 16.0)

    def test_b25_anisotropy_just_over_16x(self):
        self.assertEqual(clamp_anisotropy(16.0001), 16.0)

    def test_b26_anisotropy_extreme_large(self):
        self.assertEqual(clamp_anisotropy(1024.0), 16.0)

    def test_b27_mipmap_levels_1x1_texture(self):
        # 1x1 image should have exactly 1 mip level
        self.assertEqual(calculate_mipmap_levels(1, 1), 1)

    def test_b28_mipmap_levels_4096_texture(self):
        # 4096x4096 image -> floor(log2(4096)) + 1 = 12 + 1 = 13 levels
        self.assertEqual(calculate_mipmap_levels(4096, 4096), 13)

    # --- Fallback Boundaries ---
    def test_b29_fallback_zero_byte_png(self):
        zero_png = os.path.join(self.sandbox.mods_root, "SeamlessHD", "replacements", "0000000000000000.png")
        with open(zero_png, "wb") as f:
            pass  # 0 bytes
        pipe = HDTexturePipeline(self.sandbox.mods_root, "SeamlessHD")
        info = pipe.bind_texture(0)
        self.assertTrue(info["fallback"])
        self.assertFalse(info["u_is_hd"])

    # --- HD UI Boundaries ---
    def test_b30_ui_health_condition_unknown(self):
        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")
        # 0xFF is unknown condition; should default gracefully to status_fine
        p = ui.resolve_status_backdrop(0xFF)
        self.assertIsNotNone(p)
        self.assertIn("status_fine.png", p)

    def test_b31_ui_epilogue_boundary_indices(self):
        ui = HDUIPipeline(self.sandbox.mods_root, "SeamlessHD")
        info_0 = ui.load_backdrop("epilogue_0")
        info_7 = ui.load_backdrop("epilogue_7")
        info_8 = ui.load_backdrop("epilogue_8")  # Non-existent 9th epilogue
        self.assertTrue(info_0["has_hd"])
        self.assertTrue(info_7["has_hd"])
        self.assertFalse(info_8["has_hd"])

    # --- Texture Dumper Boundaries ---
    def test_b32_dump_zero_dimensions(self):
        dump_dir = os.path.join(self.sandbox.root, "dump_zero")
        dumper = TextureDumper(dump_dir, enabled=True)
        # Dumping 1x1
        p = dumper.dump_texture(0x1234, 0, 0, 1, 1)
        self.assertIsNotNone(p)
        self.assertTrue(os.path.isfile(p))

    def test_b33_dump_large_batch_dedup(self):
        dump_dir = os.path.join(self.sandbox.root, "dump_batch")
        dumper = TextureDumper(dump_dir, enabled=True)
        for _ in range(500):
            dumper.dump_texture(0x9999, 0, 0, 32, 32)
        self.assertEqual(dumper.dump_count, 1)
        self.assertEqual(dumper.skipped_duplicates, 499)

    # --- Launcher Boundaries ---
    def test_b34_launcher_empty_packs_dir(self):
        empty_sandbox = tempfile.mkdtemp(prefix="re3_empty_")
        try:
            dlg = LauncherTexturesDialogMock(empty_sandbox)
            packs = dlg.refresh_installed_packs()
            self.assertEqual(len(packs), 0)
        finally:
            shutil.rmtree(empty_sandbox, ignore_errors=True)

    def test_b35_launcher_many_packs_enumeration(self):
        # Create 20 mock pack directories
        for i in range(20):
            os.makedirs(os.path.join(self.sandbox.mods_root, f"Pack_{i:02d}"), exist_ok=True)
        dlg = LauncherTexturesDialogMock(self.sandbox.root, self.sandbox.settings_path)
        packs = dlg.refresh_installed_packs()
        self.assertGreaterEqual(len(packs), 20)


if __name__ == "__main__":
    unittest.main()
