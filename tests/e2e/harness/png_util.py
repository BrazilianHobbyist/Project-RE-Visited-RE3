"""
Pure Python PNG generator and validator.
Generates genuine PNG files with valid IHDR, IDAT (zlib-compressed), and IEND chunks.
Provides structure and chunk integrity validation (CRC-32, header, dimensions).
"""

import struct
import zlib
from typing import Tuple, Optional, Dict, Any

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def create_solid_rgba_png(width: int, height: int, r: int = 255, g: int = 0, b: int = 0, a: int = 255) -> bytes:
    """Create a valid PNG image byte sequence with solid RGBA color."""
    ihdr_data = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    ihdr_crc = zlib.crc32(b"IHDR" + ihdr_data) & 0xFFFFFFFF
    ihdr_chunk = struct.pack(">I", len(ihdr_data)) + b"IHDR" + ihdr_data + struct.pack(">I", ihdr_crc)

    # Raw pixel data: each scanline starts with filter byte 0 (None)
    pixel = bytes([r, g, b, a])
    raw_scanline = b"\x00" + (pixel * width)
    raw_image = raw_scanline * height

    compressed = zlib.compress(raw_image, level=6)
    idat_crc = zlib.crc32(b"IDAT" + compressed) & 0xFFFFFFFF
    idat_chunk = struct.pack(">I", len(compressed)) + b"IDAT" + compressed + struct.pack(">I", idat_crc)

    iend_crc = zlib.crc32(b"IEND") & 0xFFFFFFFF
    iend_chunk = struct.pack(">I", 0) + b"IEND" + struct.pack(">I", iend_crc)

    return PNG_SIGNATURE + ihdr_chunk + idat_chunk + iend_chunk


def create_test_pattern_png(width: int, height: int, seed: int = 42) -> bytes:
    """Create a deterministic non-uniform RGBA test pattern."""
    ihdr_data = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    ihdr_crc = zlib.crc32(b"IHDR" + ihdr_data) & 0xFFFFFFFF
    ihdr_chunk = struct.pack(">I", len(ihdr_data)) + b"IHDR" + ihdr_data + struct.pack(">I", ihdr_crc)

    scanlines = []
    for y in range(height):
        row = bytearray(1 + width * 4)
        row[0] = 0  # Filter byte None
        for x in range(width):
            offset = 1 + x * 4
            row[offset + 0] = (x * 7 + y * 13 + seed) & 0xFF
            row[offset + 1] = (x * 19 + y * 5 + seed * 3) & 0xFF
            row[offset + 2] = (x * 11 + y * 17 + seed * 7) & 0xFF
            row[offset + 3] = 255
        scanlines.append(bytes(row))
    raw_image = b"".join(scanlines)

    compressed = zlib.compress(raw_image, level=6)
    idat_crc = zlib.crc32(b"IDAT" + compressed) & 0xFFFFFFFF
    idat_chunk = struct.pack(">I", len(compressed)) + b"IDAT" + compressed + struct.pack(">I", idat_crc)

    iend_crc = zlib.crc32(b"IEND") & 0xFFFFFFFF
    iend_chunk = struct.pack(">I", 0) + b"IEND" + struct.pack(">I", iend_crc)

    return PNG_SIGNATURE + ihdr_chunk + idat_chunk + iend_chunk


def validate_png_bytes(data: bytes) -> Tuple[bool, str, Optional[Dict[str, Any]]]:
    """
    Validate PNG bytes. Returns (is_valid, reason, metadata).
    Checks signature, IHDR chunk, CRC-32 for each chunk, and IEND presence.
    """
    if len(data) < 8:
        return False, "File shorter than 8 bytes", None
    if data[:8] != PNG_SIGNATURE:
        return False, f"Invalid PNG signature: expected {PNG_SIGNATURE!r}, got {data[:8]!r}", None

    pos = 8
    chunks = []
    meta = {}
    has_ihdr = False
    has_idat = False
    has_iend = False

    while pos < len(data):
        if pos + 8 > len(data):
            return False, f"Truncated chunk header at byte {pos}", None
        length = struct.unpack(">I", data[pos : pos + 4])[0]
        ctype = data[pos + 4 : pos + 8]
        pos += 8

        if pos + length + 4 > len(data):
            return False, f"Truncated chunk data for {ctype.decode('latin1', errors='replace')}", None

        cdata = data[pos : pos + length]
        pos += length

        crc_expected = struct.unpack(">I", data[pos : pos + 4])[0]
        pos += 4

        crc_actual = zlib.crc32(ctype + cdata) & 0xFFFFFFFF
        if crc_expected != crc_actual:
            return False, f"CRC mismatch in chunk {ctype.decode('latin1')}: expected {crc_expected:#010x}, actual {crc_actual:#010x}", None

        chunks.append((ctype, length))

        if ctype == b"IHDR":
            if has_ihdr:
                return False, "Duplicate IHDR chunk", None
            has_ihdr = True
            if length != 13:
                return False, f"Invalid IHDR length: {length} != 13", None
            w, h, depth, ctype_code, comp, filter_m, interlace = struct.unpack(">IIBBBBB", cdata)
            meta["width"] = w
            meta["height"] = h
            meta["bit_depth"] = depth
            meta["color_type"] = ctype_code
            meta["compression_method"] = comp
            meta["filter_method"] = filter_m
            meta["interlace_method"] = interlace
        elif ctype == b"IDAT":
            has_idat = True
        elif ctype == b"IEND":
            has_iend = True
            break

    if not has_ihdr:
        return False, "Missing IHDR chunk", None
    if not has_idat:
        return False, "Missing IDAT chunk", None
    if not has_iend:
        return False, "Missing IEND chunk", None

    meta["chunks"] = [c[0].decode("latin1") for c in chunks]
    return True, "Valid PNG", meta
