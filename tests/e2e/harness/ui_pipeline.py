"""
HD User Interface & 2D Menus Pipeline Simulator for RE3Recomp.
Implements:
  - Full-screen HD menu backdrop replacements (inventory, status, map, epilogues, clock, save)
  - UI sprite and icon replacements via xxHash64
  - Dynamic overlay preservation
"""

import os
from typing import Optional, Dict, Any, List
from .png_util import validate_png_bytes

UI_BACKDROP_SCREENS = {
    "inventory": "inventory.png",
    "status_fine": "status_fine.png",
    "status_caution": "status_caution.png",
    "status_danger": "status_danger.png",
    "status_poison": "status_poison.png",
    "map": "map.png",
    "clock": "clock.png",
    "save": "save.png",
}

for i in range(8):
    UI_BACKDROP_SCREENS[f"epilogue_{i}"] = f"epilogue_{i}.png"


class HDUIPipeline:
    """Manages HD backdrops and sprite replacements for UI screens."""

    def __init__(self, mods_root: str, pack_name: str, enabled: bool = True):
        self.mods_root = os.path.abspath(mods_root)
        self.pack_name = pack_name
        self.enabled = enabled
        self.pack_path = os.path.join(self.mods_root, pack_name)
        self.ui_dir = os.path.join(self.pack_path, "ui")
        self.cached_backdrops: Dict[str, Dict[str, Any]] = {}
        self.cached_sprites: Dict[int, Dict[str, Any]] = {}

    def get_backdrop_path(self, screen_name: str) -> Optional[str]:
        if not self.enabled or not self.pack_name or not os.path.isdir(self.ui_dir):
            return None

        fname = UI_BACKDROP_SCREENS.get(screen_name.lower())
        if not fname:
            fname = f"{screen_name.lower()}.png"

        p = os.path.join(self.ui_dir, fname)
        if os.path.isfile(p):
            return p
        return None

    def resolve_status_backdrop(self, condition: int) -> Optional[str]:
        """Resolves condition ECG status backdrop."""
        cond_map = {
            0x04: "status_fine",
            0x03: "status_caution",
            0x02: "status_danger",
            0x01: "status_poison",
        }
        name = cond_map.get(condition, "status_fine")
        return self.get_backdrop_path(name)

    def load_backdrop(self, screen_name: str) -> Dict[str, Any]:
        """Loads and parses UI backdrop image."""
        if screen_name in self.cached_backdrops:
            return self.cached_backdrops[screen_name]

        p = self.get_backdrop_path(screen_name)
        if not p:
            res = {
                "has_hd": False,
                "screen": screen_name,
                "fallback_native_ui": True,
                "reason": "Backdrop file not found",
            }
            return res

        try:
            with open(p, "rb") as f:
                data = f.read()
            valid, reason, meta = validate_png_bytes(data)
            if not valid or not meta:
                return {
                    "has_hd": False,
                    "screen": screen_name,
                    "fallback_native_ui": True,
                    "reason": f"Corrupt backdrop PNG: {reason}",
                }

            entry = {
                "has_hd": True,
                "screen": screen_name,
                "file_path": p,
                "width": meta["width"],
                "height": meta["height"],
                "gl_tex_id": (abs(hash(screen_name)) & 0xFFFFFFFF) or 1,
                "fallback_native_ui": False,
            }
            self.cached_backdrops[screen_name] = entry
            return entry
        except Exception as e:
            return {
                "has_hd": False,
                "screen": screen_name,
                "fallback_native_ui": True,
                "reason": f"Exception: {e}",
            }

    def bind_ui_sprite(self, sprite_hash: int) -> Dict[str, Any]:
        """Looks up replacement for individual UI sprite icon (e.g. herbs, keys, font)."""
        if not self.enabled or not self.pack_name:
            return {"u_is_hd": False, "hash": sprite_hash, "fallback": True}

        if sprite_hash in self.cached_sprites:
            return self.cached_sprites[sprite_hash]

        fname = f"{sprite_hash:016x}.png"
        p = os.path.join(self.ui_dir, fname)
        if not os.path.isfile(p):
            return {"u_is_hd": False, "hash": sprite_hash, "fallback": True}

        try:
            with open(p, "rb") as f:
                data = f.read()
            valid, reason, meta = validate_png_bytes(data)
            if not valid or not meta:
                return {"u_is_hd": False, "hash": sprite_hash, "fallback": True}

            entry = {
                "u_is_hd": True,
                "hash": sprite_hash,
                "file_path": p,
                "width": meta["width"],
                "height": meta["height"],
                "gl_tex_id": (sprite_hash & 0xFFFFFFFF) or 1,
                "fallback": False,
            }
            self.cached_sprites[sprite_hash] = entry
            return entry
        except Exception:
            return {"u_is_hd": False, "hash": sprite_hash, "fallback": True}
