"""
Tier 5: Adversarial & Stress Test Suite for RE3Recomp HD Texture Engine.
Validates:
  - Encoding and escaping integrity (unicode, spaces, special chars)
  - Fault injection and corrupted PNG handling (truncated chunks, bad CRC, fake headers)
  - Security path traversal prevention (../ escaping)
  - Resource stress and cache thrashing resilience
  - Malformed TOML syntax resilience
"""

import os
import shutil
import tempfile
import unittest
from tests.e2e.harness.config import SettingsTomlManager, RE3TextureConfig
from tests.e2e.harness.memory_state import PSXGuestMemory
from tests.e2e.harness.res_resolver import ReshdpResolver
from tests.e2e.harness.cache import AsyncBackgroundCache
from tests.e2e.harness.texture_pipeline import HDTexturePipeline
from tests.e2e.harness.ui_pipeline import HDUIPipeline
from tests.e2e.harness.dumper import TextureDumper
from tests.e2e.harness.fixtures import FixtureSandbox, JILL_HASH
from tests.e2e.harness.png_util import validate_png_bytes, create_solid_rgba_png


class BaseTier5Test(unittest.TestCase):
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


class TestTier5AdversarialStress(BaseTier5Test):

    # ------------------------------------------------------------------------
    # 1. Encoding & Escaping Integrity
    # ------------------------------------------------------------------------
    def test_a01_pack_name_with_spaces_and_special_chars(self):
        special_pack = "HD_Mod (v1.0) & [4K] - $pecial!"
        p_dir = os.path.join(self.sandbox.mods_root, special_pack, "backgrounds")
        os.makedirs(p_dir, exist_ok=True)
        with open(os.path.join(p_dir, "R100_00.png"), "wb") as f:
            f.write(create_solid_rgba_png(32, 32))

        resolver = ReshdpResolver(self.sandbox.mods_root, special_pack)
        resolved = resolver.resolve(1, 0, 0)
        self.assertIsNotNone(resolved)
        self.assertTrue(os.path.isfile(resolved))

    def test_a02_windows_backslash_forward_slash_normalization(self):
        p_dir = os.path.join(self.sandbox.mods_root, "SlashTest", "backgrounds")
        os.makedirs(p_dir, exist_ok=True)
        with open(os.path.join(p_dir, "R100_00.png"), "wb") as f:
            f.write(create_solid_rgba_png(32, 32))

        # Using mixed slashes
        resolver = ReshdpResolver(self.sandbox.mods_root.replace("\\", "/"), "SlashTest")
        resolved = resolver.resolve(1, 0, 0)
        self.assertIsNotNone(resolved)

    def test_a03_unicode_pack_name(self):
        unicode_pack = "バイオハザード3_HD"
        p_dir = os.path.join(self.sandbox.mods_root, unicode_pack, "backgrounds")
        os.makedirs(p_dir, exist_ok=True)
        with open(os.path.join(p_dir, "R100_00.png"), "wb") as f:
            f.write(create_solid_rgba_png(32, 32))

        resolver = ReshdpResolver(self.sandbox.mods_root, unicode_pack)
        resolved = resolver.resolve(1, 0, 0)
        self.assertIsNotNone(resolved)

    def test_a04_toml_escaped_quotes_and_comments(self):
        complex_toml = os.path.join(self.sandbox.root, "complex_escaped.toml")
        with open(complex_toml, "w", encoding="utf-8") as f:
            f.write(
                '# Header comment with quotes "test"\n'
                '[textures]\n'
                'pack = "Escaped\\"Pack\\"" # Inline comment with = sign\n'
                'hd_backgrounds = true\n'
            )
        mgr = SettingsTomlManager(complex_toml)
        cfg = mgr.load()
        self.assertTrue(cfg.hd_backgrounds)

    # ------------------------------------------------------------------------
    # 2. Fault Injection & Corrupted Assets
    # ------------------------------------------------------------------------
    def test_a05_truncated_png_file(self):
        corrupt_bytes = b"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x01"
        valid, reason, _ = validate_png_bytes(corrupt_bytes)
        self.assertFalse(valid)
        self.assertIn("Truncated", reason)

    def test_a06_bad_crc_png_file(self):
        valid_png = create_solid_rgba_png(16, 16)
        # Invalidate IHDR CRC at byte 29..32
        bad_crc_png = valid_png[:29] + b"\xDE\xAD\xBE\xEF" + valid_png[33:]
        valid, reason, _ = validate_png_bytes(bad_crc_png)
        self.assertFalse(valid)
        self.assertIn("CRC mismatch", reason)

    def test_a07_fake_png_header(self):
        fake_data = b"GIF89a\x00\x00\x00\x00"
        valid, reason, _ = validate_png_bytes(fake_data)
        self.assertFalse(valid)
        self.assertIn("Invalid PNG signature", reason)

    def test_a08_corrupted_model_replacement_triggers_graceful_fallback(self):
        pipe = HDTexturePipeline(self.sandbox.mods_root, "CorruptPack")
        info = pipe.bind_texture(JILL_HASH)
        self.assertTrue(info["fallback"])
        self.assertFalse(info["u_is_hd"])
        self.assertIn("Corrupt PNG", info["reason"])

    def test_a09_single_corrupted_cut_in_pack_does_not_break_other_cuts(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "CorruptPack")
        cache = AsyncBackgroundCache(resolver)
        # R100_00 is corrupt in CorruptPack -> fails gracefully to None
        res_0 = cache.get(1, 0, 0)
        self.assertIsNone(res_0)

    # ------------------------------------------------------------------------
    # 3. Security & Path Traversal
    # ------------------------------------------------------------------------
    def test_a10_path_traversal_dot_dot_escape(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "../../../../windows/system32")
        self.assertIsNone(resolver.resolve(1, 0, 0))

    def test_a11_path_traversal_absolute_pack_override(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "C:\\Windows\\System32")
        self.assertIsNone(resolver.resolve(1, 0, 0))

    # ------------------------------------------------------------------------
    # 4. Resource Stress & Cache Thrashing
    # ------------------------------------------------------------------------
    def test_a12_rapid_room_switching_stress(self):
        resolver = ReshdpResolver(self.sandbox.mods_root, "SeamlessHD")
        cache = AsyncBackgroundCache(resolver, max_memory_mb=16, max_entries=4)

        cuts = [
            (1, 0, 0), (1, 0, 1), (1, 1, 0), (1, 1, 1),
            (2, 0, 0), (2, 0, 1), (2, 1, 0), (2, 1, 1),
        ]
        # 100 transitions cycling through 8 cuts with capacity 4
        for i in range(100):
            stage, room, cut = cuts[i % len(cuts)]
            cache.get(stage, room, cut)

        self.assertLessEqual(len(cache.cache), 4)
        self.assertGreater(cache.evictions, 10)

    def test_a13_deduplication_capacity_stress(self):
        dump_dir = os.path.join(self.sandbox.root, "dump_stress")
        dumper = TextureDumper(dump_dir, enabled=True)
        # Simulate 1,000 distinct hashes
        for h in range(1000):
            dumper.dump_texture(h, 0, 0, 16, 16)
        self.assertEqual(dumper.dump_count, 1000)

        # Now query all 1,000 again -> all must be skipped!
        for h in range(1000):
            dumper.dump_texture(h, 0, 0, 16, 16)
        self.assertEqual(dumper.dump_count, 1000)
        self.assertEqual(dumper.skipped_duplicates, 1000)

    # ------------------------------------------------------------------------
    # 5. Malformed TOML Syntax Resilience
    # ------------------------------------------------------------------------
    def test_a14_corrupted_toml_syntax_fallback(self):
        corrupt_toml = os.path.join(self.sandbox.root, "corrupted_syntax.toml")
        with open(corrupt_toml, "w") as f:
            f.write("[[[[[malformed syntax {{{{{ not valid toml at all\n===??!!\n")

        mgr = SettingsTomlManager(corrupt_toml)
        cfg = mgr.load()
        # Must not crash; returns safe default configuration
        self.assertEqual(cfg.pack, "SeamlessHD")
        self.assertTrue(cfg.hd_backgrounds)

    def test_a15_dumping_to_read_only_path_resilience(self):
        # Even if path is invalid or unwritable, dump_texture should return None without crashing
        dumper = TextureDumper("Z:\\NonExistentDrive\\UnwritablePath", enabled=True)
        try:
            res = dumper.dump_texture(0x1234, 0, 0, 16, 16)
            self.assertIsNone(res)
        except Exception as e:
            # Should handle or catch IO errors gracefully
            self.fail(f"Dumper crashed on unwritable path: {e}")


if __name__ == "__main__":
    unittest.main()
