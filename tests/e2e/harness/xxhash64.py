"""
Deterministic 64-bit xxHash64 implementation in pure Python.
Matches the official XXH64 specification (Yann Collet / Cyan4973).
"""

import struct

PRIME64_1 = 0x9E3779B185EBCA87
PRIME64_2 = 0xC2B2AE3D27D4EB4F
PRIME64_3 = 0x165667B19E3779F9
PRIME64_4 = 0x85EBCA77C2B2AE63
PRIME64_5 = 0x27D4EB2F165667C5
MASK64 = 0xFFFFFFFFFFFFFFFF


def _rol64(x: int, r: int) -> int:
    return ((x << r) | (x >> (64 - r))) & MASK64


def _round(acc: int, val: int) -> int:
    acc = (acc + val * PRIME64_2) & MASK64
    acc = _rol64(acc, 31)
    acc = (acc * PRIME64_1) & MASK64
    return acc


def _merge_round(acc: int, val: int) -> int:
    val = _round(0, val)
    acc = (acc ^ val) & MASK64
    acc = ((acc * PRIME64_1) + PRIME64_4) & MASK64
    return acc


class XXH64:
    def __init__(self, seed: int = 0):
        self.seed = seed & MASK64
        self.reset(seed)

    def reset(self, seed: int = 0):
        self.seed = seed & MASK64
        self.v1 = (self.seed + PRIME64_1 + PRIME64_2) & MASK64
        self.v2 = (self.seed + PRIME64_2) & MASK64
        self.v3 = self.seed & MASK64
        self.v4 = (self.seed - PRIME64_1) & MASK64
        self.total_len = 0
        self.mem = bytearray()

    def update(self, data: bytes):
        if not data:
            return self
        if isinstance(data, str):
            data = data.encode("utf-8")

        self.total_len += len(data)
        self.mem.extend(data)

        idx = 0
        limit = len(self.mem) - 32
        while idx <= limit:
            chunk = self.mem[idx : idx + 32]
            p0, p1, p2, p3 = struct.unpack("<4Q", chunk)
            self.v1 = _round(self.v1, p0)
            self.v2 = _round(self.v2, p1)
            self.v3 = _round(self.v3, p2)
            self.v4 = _round(self.v4, p3)
            idx += 32

        if idx > 0:
            self.mem = self.mem[idx:]
        return self

    def digest(self) -> int:
        if self.total_len >= 32:
            h64 = (_rol64(self.v1, 1) + _rol64(self.v2, 7) + _rol64(self.v3, 12) + _rol64(self.v4, 18)) & MASK64
            h64 = _merge_round(h64, self.v1)
            h64 = _merge_round(h64, self.v2)
            h64 = _merge_round(h64, self.v3)
            h64 = _merge_round(h64, self.v4)
        else:
            h64 = (self.seed + PRIME64_5) & MASK64

        h64 = (h64 + self.total_len) & MASK64

        mem = self.mem
        idx = 0
        rem = len(mem)

        while rem >= 8:
            k1 = struct.unpack_from("<Q", mem, idx)[0]
            k1 = _round(0, k1)
            h64 = (_rol64(h64 ^ k1, 27) * PRIME64_1 + PRIME64_4) & MASK64
            idx += 8
            rem -= 8

        if rem >= 4:
            k1 = struct.unpack_from("<I", mem, idx)[0]
            h64 = (_rol64(h64 ^ ((k1 * PRIME64_1) & MASK64), 23) * PRIME64_2 + PRIME64_3) & MASK64
            idx += 4
            rem -= 4

        while rem >= 1:
            k1 = mem[idx]
            h64 = (_rol64(h64 ^ ((k1 * PRIME64_5) & MASK64), 11) * PRIME64_1) & MASK64
            idx += 1
            rem -= 1

        # Avalanche
        h64 ^= (h64 >> 33)
        h64 = (h64 * PRIME64_2) & MASK64
        h64 ^= (h64 >> 29)
        h64 = (h64 * PRIME64_3) & MASK64
        h64 ^= (h64 >> 32)

        return h64

    def hexdigest(self) -> str:
        return f"{self.digest():016x}"


def xxhash64(data: bytes, seed: int = 0) -> int:
    h = XXH64(seed)
    h.update(data)
    return h.digest()


def xxhash64_hex(data: bytes, seed: int = 0) -> str:
    return f"{xxhash64(data, seed):016x}"
