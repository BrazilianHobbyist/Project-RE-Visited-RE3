"""
Synthetic Fixtures Generator for RE3Recomp E2E Testing.
Creates realistic mock texture packs, valid and invalid assets, and sandbox environments:
  - SeamlessHD (complete pack)
  - ClassicHD (alternative complete pack)
  - IncompletePack (partial pack to test fallback)
  - CorruptPack (corrupted PNGs to test fault injection)
"""

import os
import shutil
import tempfile
from typing import Dict, Any, Tuple
from .png_util import create_solid_rgba_png, create_test_pattern_png
from .texture_pipeline import compute_texture_xxhash64


# Known test hashes for actors
JILL_TPAGE = 0x0005
JILL_CLUT  = 0x7C00
JILL_HASH  = compute_texture_xxhash64(JILL_TPAGE, JILL_CLUT, (0, 0, 255, 255), [0x1234]*16, b"JILL_ACTOR_TEXEL_DATA_MOCK_BYTES")

NEMESIS_TPAGE = 0x0006
NEMESIS_CLUT  = 0x7C10
NEMESIS_HASH  = compute_texture_xxhash64(NEMESIS_TPAGE, NEMESIS_CLUT, (0, 0, 255, 255), [0x5678]*16, b"NEMESIS_ACTOR_TEXEL_DATA_BYTES")

WEAPON_TPAGE = 0x0007
WEAPON_CLUT  = 0x7C20
WEAPON_HASH  = compute_texture_xxhash64(WEAPON_TPAGE, WEAPON_CLUT, (0, 0, 127, 127), [0x9ABC]*16, b"WEAPON_TEXEL_DATA_MOCK_BYTES")

UI_HERB_HASH = compute_texture_xxhash64(0x0008, 0x7C30, (0, 0, 31, 31), [0x00FF]*16, b"UI_HERB_ICON_DATA")


class FixtureSandbox:
    """Manages a temporary directory tree containing settings.toml and mods/textures/."""

    def __init__(self, base_dir: str = None):
        if base_dir:
            self.root = os.path.abspath(base_dir)
            os.makedirs(self.root, exist_ok=True)
            self._is_temp = False
        else:
            self.root = tempfile.mkdtemp(prefix="re3_e2e_")
            self._is_temp = True

        self.mods_root = os.path.join(self.root, "mods", "textures")
        self.settings_path = os.path.join(self.root, "settings.toml")
        self.dump_root = os.path.join(self.root, "dump")

        self.setup_sandbox()

    def setup_sandbox(self):
        """Populates mock packs and default settings.toml."""
        os.makedirs(self.mods_root, exist_ok=True)
        os.makedirs(self.dump_root, exist_ok=True)

        self._create_pack_seamless_hd()
        self._create_pack_classic_hd()
        self._create_pack_incomplete()
        self._create_pack_corrupt()
        self._create_default_settings()

    def _create_default_settings(self):
        content = (
            "# Resident Evil 3 Recompiled Settings\n"
            "[video]\n"
            "resolution = \"1920x1080\"\n"
            "fullscreen = false\n"
            "vsync = true\n\n"
            "[audio]\n"
            "master_volume = 100\n\n"
            "[textures]\n"
            "pack = \"SeamlessHD\"\n"
            "hd_backgrounds = true\n"
            "hd_models = true\n"
            "hd_ui = true\n"
            "dump_textures = false\n"
        )
        with open(self.settings_path, "w", encoding="utf-8") as f:
            f.write(content)

    def _create_pack_seamless_hd(self):
        pack_dir = os.path.join(self.mods_root, "SeamlessHD")
        bg_dir = os.path.join(pack_dir, "backgrounds")
        rep_dir = os.path.join(pack_dir, "replacements")
        ui_dir = os.path.join(pack_dir, "ui")
        os.makedirs(bg_dir, exist_ok=True)
        os.makedirs(rep_dir, exist_ok=True)
        os.makedirs(ui_dir, exist_ok=True)

        # Backgrounds: Stage 1 (Uptown), Stage 2 (Downtown)
        rooms_and_cuts = [
            (1, 0, 0), (1, 0, 1), (1, 1, 0), (1, 1, 1),
            (2, 0, 0), (2, 0, 1), (2, 1, 0), (2, 1, 1),
        ]
        for s, r, c in rooms_and_cuts:
            fname = f"R{s:01X}{r:02X}_{c:02X}.png"
            # 1440p / 4K simulated texture (e.g. 1920x1080 or 3840x2160)
            data = create_test_pattern_png(256, 192, seed=(s * 100 + r * 10 + c))
            with open(os.path.join(bg_dir, fname), "wb") as f:
                f.write(data)

        # Also add a single-digit cut variant for testing fallback resolution: R100_2.png
        with open(os.path.join(bg_dir, "R100_2.png"), "wb") as f:
            f.write(create_solid_rgba_png(128, 96, 50, 100, 150))

        # Model Replacements
        with open(os.path.join(rep_dir, f"{JILL_HASH:016x}.png"), "wb") as f:
            f.write(create_solid_rgba_png(512, 512, 0, 128, 255))
        with open(os.path.join(rep_dir, f"{NEMESIS_HASH:016x}.png"), "wb") as f:
            f.write(create_solid_rgba_png(1024, 1024, 255, 30, 30))
        with open(os.path.join(rep_dir, f"{WEAPON_HASH:016x}.png"), "wb") as f:
            f.write(create_solid_rgba_png(256, 256, 128, 128, 128))

        # UI Backdrops
        ui_screens = [
            "inventory.png", "status_fine.png", "status_caution.png",
            "status_danger.png", "status_poison.png", "map.png",
            "clock.png", "save.png"
        ]
        for name in ui_screens:
            with open(os.path.join(ui_dir, name), "wb") as f:
                f.write(create_solid_rgba_png(640, 480, 20, 40, 60))

        for i in range(8):
            with open(os.path.join(ui_dir, f"epilogue_{i}.png"), "wb") as f:
                f.write(create_solid_rgba_png(320, 240, 70 + i * 10, 50, 90))

        # UI Sprite
        with open(os.path.join(ui_dir, f"{UI_HERB_HASH:016x}.png"), "wb") as f:
            f.write(create_solid_rgba_png(64, 64, 0, 255, 0))

    def _create_pack_classic_hd(self):
        pack_dir = os.path.join(self.mods_root, "ClassicHD")
        bg_dir = os.path.join(pack_dir, "backgrounds")
        os.makedirs(bg_dir, exist_ok=True)
        with open(os.path.join(bg_dir, "R100_00.png"), "wb") as f:
            f.write(create_solid_rgba_png(320, 240, 10, 80, 20))

    def _create_pack_incomplete(self):
        # Pack with only one background and no replacements or UI
        pack_dir = os.path.join(self.mods_root, "IncompletePack")
        bg_dir = os.path.join(pack_dir, "backgrounds")
        os.makedirs(bg_dir, exist_ok=True)
        with open(os.path.join(bg_dir, "R100_00.png"), "wb") as f:
            f.write(create_solid_rgba_png(320, 240, 200, 200, 200))

    def _create_pack_corrupt(self):
        pack_dir = os.path.join(self.mods_root, "CorruptPack")
        bg_dir = os.path.join(pack_dir, "backgrounds")
        rep_dir = os.path.join(pack_dir, "replacements")
        os.makedirs(bg_dir, exist_ok=True)
        os.makedirs(rep_dir, exist_ok=True)

        # Corrupt PNG: Truncated header
        with open(os.path.join(bg_dir, "R100_00.png"), "wb") as f:
            f.write(b"\x89PNG\r\n\x1a\nTRUNCATED_GARBAGE_BYTES")

        # Corrupt PNG: Invalid CRC in IHDR
        valid = create_solid_rgba_png(32, 32, 255, 0, 0)
        corrupted_crc = valid[:29] + b"\xDE\xAD\xBE\xEF" + valid[33:]
        with open(os.path.join(rep_dir, f"{JILL_HASH:016x}.png"), "wb") as f:
            f.write(corrupted_crc)

    def cleanup(self):
        if self._is_temp and os.path.exists(self.root):
            try:
                shutil.rmtree(self.root, ignore_errors=True)
            except OSError:
                pass
