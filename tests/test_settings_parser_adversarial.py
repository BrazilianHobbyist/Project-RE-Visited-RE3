"""
Python adversarial test comparing SettingsTomlManager vs C++ runtime parser behavior.
"""

import os
import unittest
from tests.e2e.harness.config import SettingsTomlManager, RE3TextureConfig

class TestSettingsParserComparison(unittest.TestCase):
    def setUp(self):
        self.test_file = os.path.abspath("build/test_adversarial_settings.toml")
        os.makedirs("build", exist_ok=True)

    def tearDown(self):
        if os.path.exists(self.test_file):
            os.remove(self.test_file)

    def test_single_quotes_in_python_mock(self):
        with open(self.test_file, "w") as f:
            f.write("[textures]\npack = 'SingleQuotePack'\n")
        mgr = SettingsTomlManager(self.test_file)
        cfg = mgr.load()
        # In Python mock, single quotes are stripped:
        self.assertEqual(cfg.pack, "SingleQuotePack")

    def test_boolean_case_insensitivity_in_python_mock(self):
        with open(self.test_file, "w") as f:
            f.write("[textures]\ndump_textures = True\nhd_backgrounds = TRUE\n")
        mgr = SettingsTomlManager(self.test_file)
        cfg = mgr.load()
        # In Python mock, True and TRUE evaluate to True:
        self.assertTrue(cfg.dump_textures)
        self.assertTrue(cfg.hd_backgrounds)

    def test_inline_comments_in_python_mock(self):
        with open(self.test_file, "w") as f:
            f.write("[textures]\npack = \"CommentPack\" # inline comment\nhd_backgrounds = true # inline comment\n")
        mgr = SettingsTomlManager(self.test_file)
        cfg = mgr.load()
        # In Python mock, inline comments are stripped:
        self.assertEqual(cfg.pack, "CommentPack")
        self.assertTrue(cfg.hd_backgrounds)

if __name__ == "__main__":
    unittest.main()
