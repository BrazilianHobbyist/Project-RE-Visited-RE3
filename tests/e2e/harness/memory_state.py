"""
Emulated PSX Guest Memory and State Tracker for RE3Recomp.
Models the authoritative memory locations identified in surveys:
  - 0x000D1F08: Stage (uint8_t)
  - 0x000D1F09: Room  (uint8_t)
  - 0x000D1F0A: Cut   (uint8_t)
  - 0x000CCC97: Player Condition (0x04=Fine, 0x03=Caution, 0x02=Danger, 0x01=Poison)
  - 0x000D1F3E: Epilogue Count (0..8)
  - 0x000D225E: Inventory Max Slots (8 or 10)
  - 0x000D2134: Inventory Item Slots Base
"""

from typing import Tuple, Dict

ADDR_STAGE = 0x000D1F08
ADDR_ROOM  = 0x000D1F09
ADDR_CUT   = 0x000D1F0A
ADDR_PLAYER_CONDITION = 0x000CCC97
ADDR_EPILOGUE_COUNT   = 0x000D1F3E
ADDR_INVENTORY_MAX    = 0x000D225E
ADDR_INVENTORY_SLOTS  = 0x000D2134

CONDITION_FINE    = 0x04
CONDITION_CAUTION = 0x03
CONDITION_DANGER  = 0x02
CONDITION_POISON  = 0x01


class PSXGuestMemory:
    """Simulates the 2MB PSX guest RAM addressing and memory-mapped IO."""

    RAM_SIZE = 2 * 1024 * 1024  # 2MB
    RAM_MASK = 0x001FFFFF

    def __init__(self):
        self.ram = bytearray(self.RAM_SIZE)
        self.reset()

    def reset(self):
        for i in range(len(self.ram)):
            self.ram[i] = 0
        # Defaults: Stage 1 (Uptown), Room 0, Cut 0, Fine condition
        self.write_byte(ADDR_STAGE, 1)
        self.write_byte(ADDR_ROOM, 0)
        self.write_byte(ADDR_CUT, 0)
        self.write_byte(ADDR_PLAYER_CONDITION, CONDITION_FINE)
        self.write_byte(ADDR_EPILOGUE_COUNT, 0)
        self.write_byte(ADDR_INVENTORY_MAX, 8)

    def _phys(self, addr: int) -> int:
        return (addr & 0x1FFFFFFF) & self.RAM_MASK

    def read_byte(self, addr: int) -> int:
        return self.ram[self._phys(addr)]

    def write_byte(self, addr: int, val: int) -> None:
        self.ram[self._phys(addr)] = val & 0xFF

    def read_u16(self, addr: int) -> int:
        p = self._phys(addr)
        return self.ram[p] | (self.ram[p + 1] << 8)

    def write_u16(self, addr: int, val: int) -> None:
        p = self._phys(addr)
        self.ram[p] = val & 0xFF
        self.ram[p + 1] = (val >> 8) & 0xFF

    def read_u32(self, addr: int) -> int:
        p = self._phys(addr)
        return (
            self.ram[p]
            | (self.ram[p + 1] << 8)
            | (self.ram[p + 2] << 16)
            | (self.ram[p + 3] << 24)
        )

    def write_u32(self, addr: int, val: int) -> None:
        p = self._phys(addr)
        self.ram[p] = val & 0xFF
        self.ram[p + 1] = (val >> 8) & 0xFF
        self.ram[p + 2] = (val >> 16) & 0xFF
        self.ram[p + 3] = (val >> 24) & 0xFF

    def set_camera(self, stage: int, room: int, cut: int) -> None:
        self.write_byte(ADDR_STAGE, stage)
        self.write_byte(ADDR_ROOM, room)
        self.write_byte(ADDR_CUT, cut)

    def get_camera(self) -> Tuple[int, int, int]:
        return (
            self.read_byte(ADDR_STAGE),
            self.read_byte(ADDR_ROOM),
            self.read_byte(ADDR_CUT),
        )

    def set_player_condition(self, condition: int) -> None:
        self.write_byte(ADDR_PLAYER_CONDITION, condition)

    def get_player_condition(self) -> int:
        return self.read_byte(ADDR_PLAYER_CONDITION)
