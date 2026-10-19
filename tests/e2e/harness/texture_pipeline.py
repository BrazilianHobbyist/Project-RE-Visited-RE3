"""
3D Model & Actor HD Texture Replacement Pipeline Simulator for RE3Recomp.
Implements:
  - 64-bit xxHash64 composite fingerprint computation
  - UV normalization formulas (full page and sub-rectangle)
  - Anisotropic & trilinear filtering configuration
  - Replacement lookup in mods/textures/<pack>/replacements/<hash>.png
  - Transparent PS1 VRAM fallback mechanics
  - Foreground occlusion mask screen-space alignment
"""

import math
import os
import struct
from typing import Optional, Tuple, List, Dict, Any
from .xxhash64 import xxhash64
from .png_util import validate_png_bytes


def compute_texture_xxhash64(
    tpage: int,
    clut: int,
    limits: Optional[Tuple[int, int, int, int]] = None,
    palette: Optional[List[int]] = None,
    vram_texels: Optional[bytes] = None,
) -> int:
    """
    Computes deterministic 64-bit xxHash64 for composite PS1 texture payload.
    Metadata (16B): tpage(u16), clut(u16), u_min(u16), v_min(u16), u_max(u16), v_max(u16), flags(u32).
    Palette: 16 words (4-bit), 256 words (8-bit), or 0 (direct 15-bit).
    Texels: Raw VRAM words in bounding rectangle.
    """
    u_min, v_min, u_max, v_max = limits if limits else (0, 0, 255, 255)
    meta_bytes = struct.pack("<HHHHHHI", tpage, clut, u_min, v_min, u_max, v_max, 0)

    payload = bytearray(meta_bytes)

    depth = (tpage >> 7) & 3
    if depth < 2:
        pal_count = 16 if depth == 0 else 256
        if palette:
            for i in range(min(len(palette), pal_count)):
                payload.extend(struct.pack("<H", palette[i] & 0xFFFF))
            # Pad if shorter
            for _ in range(max(0, pal_count - len(palette))):
                payload.extend(b"\x00\x00")
        else:
            payload.extend(b"\x00" * (pal_count * 2))

    if vram_texels:
        payload.extend(vram_texels)

    return xxhash64(bytes(payload), seed=0)


def normalize_uv_full_page(u: float, v: float, page_size: float = 256.0) -> Tuple[float, float]:
    """
    Scenario A: Full Texture Page Replacement (256x256 PS1 -> Normalized [0, 1]).
    Centers half-texel to eliminate 1-pixel edge bleeding.
    Formula: (u + 0.5) / 256.0, (v + 0.5) / 256.0
    """
    u_norm = (u + 0.5) / page_size
    v_norm = (v + 0.5) / page_size
    return u_norm, v_norm


def normalize_uv_sub_rect(
    u: float, v: float, u_min: int, v_min: int, u_max: int, v_max: int
) -> Tuple[float, float]:
    """
    Scenario B: Bounded Sub-Rectangle Replacement.
    Formula: (u - u_min) / (u_max - u_min + 1)
    """
    w = max(1, u_max - u_min + 1)
    h = max(1, v_max - v_min + 1)
    u_norm = (u - u_min) / float(w)
    v_norm = (v - v_min) / float(h)
    return u_norm, v_norm


def calculate_mipmap_levels(width: int, height: int) -> int:
    """Calculates number of trilinear mipmap levels: floor(log2(max(w, h))) + 1."""
    if width <= 0 or height <= 0:
        return 1
    max_dim = max(width, height)
    return int(math.floor(math.log2(max_dim))) + 1


def clamp_anisotropy(requested_aniso: float, max_supported: float = 16.0) -> float:
    """Clamps requested anisotropic filtering to hardware maximum (typically 16x)."""
    return max(1.0, min(float(requested_aniso), float(max_supported)))


class HDTexturePipeline:
    """Simulates HD model texture replacement, binding, and fallback."""

    def __init__(self, mods_root: str, pack_name: str, enabled: bool = True, requested_aniso: float = 16.0):
        self.mods_root = os.path.abspath(mods_root)
        self.pack_name = pack_name
        self.enabled = enabled
        self.pack_path = os.path.join(self.mods_root, pack_name)
        self.replacements_dir = os.path.join(self.pack_path, "replacements")
        self.aniso = clamp_anisotropy(requested_aniso, 16.0)
        self.loaded_textures: Dict[int, Dict[str, Any]] = {}

    def get_replacement_path(self, hash_val: int) -> str:
        fname = f"{hash_val:016x}.png"
        return os.path.join(self.replacements_dir, fname)

    def bind_texture(self, hash_val: int) -> Dict[str, Any]:
        """
        Binds replacement texture if found and enabled;
        otherwise returns fallback state.
        """
        if not self.enabled or not self.pack_name:
            return {
                "u_is_hd": False,
                "gl_tex_id": 0,
                "hash": hash_val,
                "fallback": True,
                "reason": "Disabled or no pack",
            }

        if hash_val in self.loaded_textures:
            return self.loaded_textures[hash_val]

        p = self.get_replacement_path(hash_val)
        if not os.path.isfile(p):
            # Fallback to PS1 VRAM sampling
            result = {
                "u_is_hd": False,
                "gl_tex_id": 0,
                "hash": hash_val,
                "fallback": True,
                "reason": "File not found",
            }
            return result

        try:
            with open(p, "rb") as f:
                data = f.read()
            valid, reason, meta = validate_png_bytes(data)
            if not valid or not meta:
                return {
                    "u_is_hd": False,
                    "gl_tex_id": 0,
                    "hash": hash_val,
                    "fallback": True,
                    "reason": f"Corrupt PNG: {reason}",
                }

            w, h = meta["width"], meta["height"]
            mipmaps = calculate_mipmap_levels(w, h)
            tex_entry = {
                "u_is_hd": True,
                "gl_tex_id": (hash_val & 0xFFFFFFFF) or 1,
                "hash": hash_val,
                "file_path": p,
                "width": w,
                "height": h,
                "aniso": self.aniso,
                "mipmaps": mipmaps,
                "min_filter": "GL_LINEAR_MIPMAP_LINEAR",
                "mag_filter": "GL_LINEAR",
                "fallback": False,
            }
            self.loaded_textures[hash_val] = tex_entry
            return tex_entry
        except Exception as e:
            return {
                "u_is_hd": False,
                "gl_tex_id": 0,
                "hash": hash_val,
                "fallback": True,
                "reason": f"Exception loading file: {e}",
            }


def compute_occlusion_mask_alignment(
    quad_x: int, quad_y: int, quad_w: int, quad_h: int, fbo_scale: int = 4
) -> Dict[str, Any]:
    """
    Computes screen-space UVs and FBO destination rectangle for foreground occlusion mask quads.
    Screen is 320x240 PS1 coordinate space.
    """
    u_min = quad_x / 320.0
    v_min = quad_y / 240.0
    u_max = (quad_x + quad_w) / 320.0
    v_max = (quad_y + quad_h) / 240.0

    dst_x = quad_x * fbo_scale
    dst_y = quad_y * fbo_scale
    dst_w = quad_w * fbo_scale
    dst_h = quad_h * fbo_scale

    return {
        "screen_uv": (u_min, v_min, u_max, v_max),
        "fbo_rect": (dst_x, dst_y, dst_w, dst_h),
        "fbo_scale": fbo_scale,
        "is_aligned": (
            0.0 <= u_min <= 1.0 and 0.0 <= v_min <= 1.0 and
            u_max <= 1.05 and v_max <= 1.05
        ),
    }
