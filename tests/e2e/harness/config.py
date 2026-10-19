"""
Configuration parser and serializer for settings.toml [textures] section.
Matches the contract in PROJECT.md and runtime load_re3_qol_settings() / config_loader.cpp.
Preserves existing sections and comments when updating.
"""

from dataclasses import dataclass, field
from typing import Dict, Any, Optional
import os
import re


@dataclass
class RE3TextureConfig:
    pack: str = "SeamlessHD"
    hd_backgrounds: bool = True
    hd_models: bool = True
    hd_ui: bool = True
    dump_textures: bool = False

    def to_dict(self) -> Dict[str, Any]:
        return {
            "pack": self.pack,
            "hd_backgrounds": self.hd_backgrounds,
            "hd_models": self.hd_models,
            "hd_ui": self.hd_ui,
            "dump_textures": self.dump_textures,
        }


class SettingsTomlManager:
    """Manages reading, parsing, updating, and writing settings.toml."""

    def __init__(self, file_path: str):
        self.file_path = os.path.abspath(file_path)
        self.raw_text: str = ""
        self.sections: Dict[str, Dict[str, str]] = {}
        self.texture_config: RE3TextureConfig = RE3TextureConfig()
        if os.path.exists(self.file_path):
            self.load()

    def load(self) -> RE3TextureConfig:
        """Load and parse the settings.toml file."""
        if not os.path.exists(self.file_path):
            self.texture_config = RE3TextureConfig()
            return self.texture_config

        with open(self.file_path, "r", encoding="utf-8", errors="replace") as f:
            self.raw_text = f.read()

        current_section = ""
        self.sections.clear()
        self.sections[""] = {}

        for line in self.raw_text.splitlines():
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue

            sec_match = re.match(r"^\[\s*([a-zA-Z0-9_\-\.]+)\s*\]", stripped)
            if sec_match:
                current_section = sec_match.group(1).lower()
                if current_section not in self.sections:
                    self.sections[current_section] = {}
                continue

            if "=" in stripped:
                parts = stripped.split("=", 1)
                k = parts[0].strip().lower()
                v = parts[1].strip()
                # Strip inline comment
                if "#" in v and not (v.startswith('"') and v.endswith('"')):
                    v = v.split("#", 1)[0].strip()
                self.sections[current_section][k] = v

        # Parse [textures] section
        tex_sec = self.sections.get("textures", {})
        pack_val = tex_sec.get("pack", "SeamlessHD")
        if pack_val.startswith('"') and pack_val.endswith('"'):
            pack_val = pack_val[1:-1]
        elif pack_val.startswith("'") and pack_val.endswith("'"):
            pack_val = pack_val[1:-1]

        def _to_bool(val: str, default: bool) -> bool:
            v_low = val.strip().lower()
            if v_low in ("true", "1", "yes", "on"):
                return True
            if v_low in ("false", "0", "no", "off"):
                return False
            return default

        self.texture_config = RE3TextureConfig(
            pack=pack_val,
            hd_backgrounds=_to_bool(tex_sec.get("hd_backgrounds", "true"), True),
            hd_models=_to_bool(tex_sec.get("hd_models", "true"), True),
            hd_ui=_to_bool(tex_sec.get("hd_ui", "true"), True),
            dump_textures=_to_bool(tex_sec.get("dump_textures", "false"), False),
        )
        return self.texture_config

    def save(self, config: Optional[RE3TextureConfig] = None) -> None:
        """Write texture config back to file, preserving or creating [textures] section."""
        if config is not None:
            self.texture_config = config

        cfg = self.texture_config
        new_block = (
            f"[textures]\n"
            f'pack = "{cfg.pack}"\n'
            f"hd_backgrounds = {'true' if cfg.hd_backgrounds else 'false'}\n"
            f"hd_models = {'true' if cfg.hd_models else 'false'}\n"
            f"hd_ui = {'true' if cfg.hd_ui else 'false'}\n"
            f"dump_textures = {'true' if cfg.dump_textures else 'false'}\n"
        )

        if not os.path.exists(self.file_path):
            os.makedirs(os.path.dirname(self.file_path), exist_ok=True)
            with open(self.file_path, "w", encoding="utf-8") as f:
                f.write(new_block)
            return

        with open(self.file_path, "r", encoding="utf-8", errors="replace") as f:
            content = f.read()

        # Check if [textures] already exists
        tex_pattern = re.compile(r"\[\s*textures\s*\][\s\S]*?(?=(\n\[|\Z))", re.IGNORECASE)
        if tex_pattern.search(content):
            updated_content = tex_pattern.sub(new_block.strip() + "\n", content)
        else:
            updated_content = content.rstrip() + "\n\n" + new_block

        with open(self.file_path, "w", encoding="utf-8") as f:
            f.write(updated_content)
        self.raw_text = updated_content
