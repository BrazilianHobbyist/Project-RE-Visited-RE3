import os
import struct

def find_zombies_in_all_rooms():
    txt_path = "disc_file_list.txt" if os.path.exists("disc_file_list.txt") else os.path.join(os.path.dirname(__file__), "disc_file_list.txt")
    with open(txt_path, "r", encoding="utf-8") as f:
        all_files = [line.strip().split("\t") for line in f]
        
    s1_bins = [parts for parts in all_files if parts[0].startswith("CD_DATA/STAGE1/R") and parts[0].endswith(".BIN")]
    print(f"Found {len(s1_bins)} room BIN files in STAGE1")
    
    bin_path = r"Resident Evil 3 - dados\Resident Evil 3 - Nemesis (USA, Canada).bin"
    if not os.path.exists(bin_path):
        bin_path = r"ResidentEvil3-PCPort\gamedata\Resident Evil 3 - Nemesis (USA, Canada).bin"
        
    with open(bin_path, "rb") as bin_f:
        for fname, lba_s, size_s, raw_s in s1_bins:
            lba = int(lba_s)
            size = int(size_s)
            # Read first 100 bytes (header)
            bin_f.seek(lba * 2352 + 24)
            head = bin_f.read(min(size, 2048))
            if len(head) < 8: continue
            total_sz, count = struct.unpack('<II', head[:8])
            if count > 20: continue # not a standard room container
            
            enemies = []
            for i in range(count):
                e_sz, e_id, e_type, e_extra = struct.unpack('<IBBH', head[8+i*8:8+i*8+8])
                if e_type == 0 and e_extra in (0x5495, 0x59F3, 0x5400, 0x5900) or (e_extra >> 8) in (0x54, 0x59, 0x10, 0x11, 0x12):
                    enemies.append(f"0x{e_extra>>8:02X}")
            if enemies:
                print(f"  {fname:25} | LBA: {lba:6} | Enemies: {enemies}")

if __name__ == "__main__":
    find_zombies_in_all_rooms()
