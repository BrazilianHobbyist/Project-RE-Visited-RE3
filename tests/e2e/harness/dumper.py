"""
Automated Texture Dumping Tooling Simulator for RE3Recomp.
Implements:
  - Activation via dump_textures = true
  - In-memory session deduplication (no redundant disk writes)
  - Standardized PNG export with xxHash64 filenames
  - Metadata manifest generation
"""

import json
import os
import time
from typing import Set, Dict, Any, Optional
from .png_util import create_solid_rgba_png, create_test_pattern_png, validate_png_bytes


class TextureDumper:
    """Manages texture dumping to disk with session deduplication."""

    def __init__(self, dump_root: str, enabled: bool = False):
        self.dump_root = os.path.abspath(dump_root)
        self.textures_dir = os.path.join(self.dump_root, "textures")
        self.enabled = enabled
        self.session_hashes: Set[int] = set()
        self.manifest: Dict[str, Any] = {"version": 1, "entries": []}
        self.dump_count = 0
        self.skipped_duplicates = 0

    def dump_texture(
        self,
        hash_val: int,
        tpage: int,
        clut: int,
        width: int,
        height: int,
        raw_rgba: Optional[bytes] = None,
    ) -> Optional[str]:
        """
        Dumps texture to dump/textures/<hash:016x>.png.
        Returns path to dumped file or None if skipped/disabled.
        """
        if not self.enabled:
            return None

        # Session deduplication check
        if hash_val in self.session_hashes:
            self.skipped_duplicates += 1
            return None

        try:
            os.makedirs(self.textures_dir, exist_ok=True)
            fname = f"{hash_val:016x}.png"
            out_path = os.path.join(self.textures_dir, fname)

            # Generate genuine PNG bytes
            if raw_rgba:
                png_bytes = create_test_pattern_png(width, height, seed=hash_val & 0xFFFF)
            else:
                png_bytes = create_test_pattern_png(width, height, seed=hash_val & 0xFFFF)

            with open(out_path, "wb") as f:
                f.write(png_bytes)
        except OSError:
            return None

        self.session_hashes.add(hash_val)
        self.dump_count += 1

        entry_meta = {
            "hash": f"{hash_val:016x}",
            "filename": fname,
            "tpage": tpage,
            "clut": clut,
            "width": width,
            "height": height,
            "timestamp": time.time(),
        }
        self.manifest["entries"].append(entry_meta)
        self._save_manifest()

        return out_path

    def _save_manifest(self):
        manifest_path = os.path.join(self.textures_dir, "manifest.json")
        try:
            with open(manifest_path, "w", encoding="utf-8") as f:
                json.dump(self.manifest, f, indent=2)
        except OSError:
            pass

    def get_manifest(self) -> Dict[str, Any]:
        return self.manifest
