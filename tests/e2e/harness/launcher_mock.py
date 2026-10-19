"""
Win32 Launcher Simulation and Persistence Validator for RE3Recomp.
Implements:
  - Scanning mods/textures/ for installed packs
  - UI state management (pack selection, feature checkboxes)
  - Settings persistence to settings.toml without corrupting other sections
  - Open mods folder path validation
"""

import os
from typing import List, Dict, Any, Optional
from .config import SettingsTomlManager, RE3TextureConfig


class LauncherTexturesDialogMock:
    """Simulates the Win32 ShowTexturesDialog in Resident_Evil_3_Launcher.exe."""

    def __init__(self, project_root: str, settings_path: Optional[str] = None):
        self.project_root = os.path.abspath(project_root)
        self.settings_path = (
            os.path.abspath(settings_path)
            if settings_path
            else os.path.join(self.project_root, "settings.toml")
        )
        self.mods_textures_dir = os.path.join(self.project_root, "mods", "textures")
        self.config_manager = SettingsTomlManager(self.settings_path)

        # UI State
        self.installed_packs: List[str] = []
        self.selected_pack: str = ""
        self.chk_hd_backgrounds: bool = True
        self.chk_hd_models: bool = True
        self.chk_hd_ui: bool = True
        self.chk_dump_textures: bool = False

        self.refresh_installed_packs()
        self.load_from_settings()

    def refresh_installed_packs(self) -> List[str]:
        """Scans mods/textures/ directory for valid texture pack folders."""
        self.installed_packs = []
        if os.path.isdir(self.mods_textures_dir):
            try:
                for entry in sorted(os.listdir(self.mods_textures_dir)):
                    full_p = os.path.join(self.mods_textures_dir, entry)
                    if os.path.isdir(full_p) and not entry.startswith("."):
                        self.installed_packs.append(entry)
            except OSError:
                pass
        return self.installed_packs

    def load_from_settings(self):
        """Loads configuration from settings.toml into UI controls."""
        cfg = self.config_manager.load()
        self.selected_pack = cfg.pack
        self.chk_hd_backgrounds = cfg.hd_backgrounds
        self.chk_hd_models = cfg.hd_models
        self.chk_hd_ui = cfg.hd_ui
        self.chk_dump_textures = cfg.dump_textures

    def save_to_settings(self):
        """Persists UI control states back to settings.toml."""
        cfg = RE3TextureConfig(
            pack=self.selected_pack,
            hd_backgrounds=self.chk_hd_backgrounds,
            hd_models=self.chk_hd_models,
            hd_ui=self.chk_hd_ui,
            dump_textures=self.chk_dump_textures,
        )
        self.config_manager.save(cfg)

    def select_pack(self, pack_name: str) -> bool:
        """Selects a pack if it exists or allows empty string to disable."""
        if pack_name == "" or pack_name in self.installed_packs:
            self.selected_pack = pack_name
            return True
        return False

    def get_mods_folder_path(self) -> str:
        """Path used by the 'Open Mods Folder' button."""
        os.makedirs(self.mods_textures_dir, exist_ok=True)
        return self.mods_textures_dir
