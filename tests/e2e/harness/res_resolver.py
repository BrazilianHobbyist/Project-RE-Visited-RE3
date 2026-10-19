"""
RESHDP Background Asset Resolver for RE3Recomp.
Implements the standard Seamless HD Project resolution rules:
  1. Standard RESHDP: R<stage><room:02X>_<cut:02X>.png (e.g. R100_00.png)
  2. Single-digit cut: R<stage><room:02X>_<cut>.png (e.g. R100_0.png)
  3. Case-insensitive filesystem fallback
  4. Path traversal protection
"""

import os
from typing import Optional, List, Tuple


class ReshdpResolver:
    """Resolves background image paths for given (stage, room, cut) in a pack."""

    def __init__(self, mods_root: str, pack_name: str):
        self.mods_root = os.path.abspath(mods_root)
        self.pack_name = pack_name
        self.pack_path = os.path.join(self.mods_root, pack_name)
        self.bg_dir = os.path.join(self.pack_path, "backgrounds")

    @staticmethod
    def get_candidate_filenames(stage: int, room: int, cut: int) -> List[str]:
        """Generate filename candidates in order of priority."""
        candidates = []
        # Standard RESHDP format: R<stage><room:02X>_<cut:02X>.png
        c1 = f"R{stage:01X}{room:02X}_{cut:02X}.png"
        candidates.append(c1)

        # Single-digit cut format: R<stage><room:02X>_<cut>.png
        c2 = f"R{stage:01X}{room:02X}_{cut}.png"
        if c2 not in candidates:
            candidates.append(c2)

        # Lowercase variations
        c3 = c1.lower()
        if c3 not in candidates:
            candidates.append(c3)

        c4 = c2.lower()
        if c4 not in candidates:
            candidates.append(c4)

        return candidates

    def resolve(self, stage: int, room: int, cut: int) -> Optional[str]:
        """
        Locates the background file on disk if it exists.
        Returns absolute file path or None if not found (for fallback).
        """
        if not self.pack_name or not os.path.isdir(self.bg_dir):
            return None

        # Guard against path traversal in pack_name
        norm_bg_dir = os.path.normpath(self.bg_dir)
        if not norm_bg_dir.startswith(self.mods_root):
            return None

        candidates = self.get_candidate_filenames(stage, room, cut)

        # First check direct candidates
        for cand in candidates:
            p = os.path.join(self.bg_dir, cand)
            if os.path.isfile(p):
                return os.path.abspath(p)

        # Case-insensitive filesystem scan within backgrounds folder
        try:
            dir_files = os.listdir(self.bg_dir)
            dir_map = {f.lower(): f for f in dir_files}
            for cand in candidates:
                cand_lower = cand.lower()
                if cand_lower in dir_map:
                    return os.path.abspath(os.path.join(self.bg_dir, dir_map[cand_lower]))
        except OSError:
            return None

        return None

    def list_room_cuts(self, stage: int, room: int) -> List[Tuple[int, str]]:
        """List all available cut files for a given stage and room (for pre-fetching)."""
        if not os.path.isdir(self.bg_dir):
            return []

        prefix = f"r{stage:01x}{room:02x}_"
        results = []
        try:
            for fname in os.listdir(self.bg_dir):
                fl = fname.lower()
                if fl.startswith(prefix) and fl.endswith(".png"):
                    # Extract cut number
                    cut_part = fl[len(prefix):-4]
                    try:
                        cut_val = int(cut_part, 16)
                        results.append((cut_val, os.path.join(self.bg_dir, fname)))
                    except ValueError:
                        try:
                            cut_val = int(cut_part, 10)
                            results.append((cut_val, os.path.join(self.bg_dir, fname)))
                        except ValueError:
                            continue
        except OSError:
            pass

        return sorted(results, key=lambda x: x[0])
