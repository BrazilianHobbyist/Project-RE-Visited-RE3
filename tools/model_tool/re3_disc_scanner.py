import os
import struct

def scan_iso(bin_path):
    print(f"Scanning ISO: {bin_path}")
    with open(bin_path, "rb") as f:
        # Check PVD at sector 16 (2048 * 16 = 0x8000 for standard ISO, but PS1 BIN is sector 2352)
        # Determine sector size: 2352 (RAW) or 2048 (ISO)
        f.seek(0, 2)
        total_size = f.tell()
        
        # Test RAW (2352): sector 16 sync pattern + header -> 16 * 2352 = 37632 (0x9300)
        # In mode 2 form 1: 16 bytes sync + header, data starts at 0x9318 (offset 24 in sector)
        # In mode 1: 16 bytes sync + header, data starts at 0x9310
        # Let's search for "CD001"
        f.seek(0)
        pvd_offset = -1
        sector_size = 2352
        data_offset_in_sector = 24 # standard Mode 2 Form 1
        
        # Let's scan first 32 sectors
        for s in range(16, 25):
            for off in [s * 2352 + 16, s * 2352 + 24, s * 2048]:
                if off + 5 <= total_size:
                    f.seek(off)
                    magic = f.read(5)
                    if magic == b"CD001":
                        pvd_offset = off
                        if "2352" in str(off):
                            pass
                        print(f"Found PVD magic CD001 at offset {hex(off)}")
                        break
            if pvd_offset != -1:
                break
                
        if pvd_offset == -1:
            print("PVD magic not found in standard locations, searching first 1MB...")
            f.seek(0)
            chunk = f.read(1024 * 1024)
            idx = chunk.find(b"CD001")
            if idx != -1:
                print(f"Found CD001 at {hex(idx)}")
                pvd_offset = idx
            else:
                print("Could not find ISO PVD.")
                return

        # Determine sector layout
        # If idx == 0x9318 -> 16 * 2352 + 24 -> raw sector 2352, data offset 24
        sector_size = 2352 if total_size % 2352 == 0 else 2048
        data_sub_offset = 24 if sector_size == 2352 else 0
        if sector_size == 2352 and (pvd_offset % 2352 == 16):
            data_sub_offset = 16

        def read_sector_data(lba, count=1):
            buf = bytearray()
            for i in range(count):
                cur_lba = lba + i
                if sector_size == 2352:
                    f.seek(cur_lba * 2352 + data_sub_offset)
                    buf.extend(f.read(2048))
                else:
                    f.seek(cur_lba * 2048)
                    buf.extend(f.read(2048))
            return bytes(buf)

        # In PVD, root directory record is at offset 156 from start of PVD (pvd_offset - 1 for type code)
        # PVD starts at pvd_offset - 1
        f.seek(pvd_offset - 1 + 156)
        root_rec = f.read(34)
        root_lba = struct.unpack("<I", root_rec[2:6])[0]
        root_size = struct.unpack("<I", root_rec[10:14])[0]
        print(f"Root dir LBA: {root_lba}, Size: {root_size}")

        all_files = []

        def parse_dir(lba, size, current_path=""):
            sec_count = (size + 2047) // 2048
            data = read_sector_data(lba, sec_count)
            pos = 0
            while pos < size and pos < len(data):
                rec_len = data[pos]
                if rec_len == 0:
                    # Sector boundary padding
                    pos = ((pos // 2048) + 1) * 2048
                    continue
                if pos + rec_len > len(data):
                    break
                rec = data[pos:pos+rec_len]
                file_lba = struct.unpack("<I", rec[2:6])[0]
                file_size = struct.unpack("<I", rec[10:14])[0]
                flags = rec[25]
                name_len = rec[32]
                name = rec[33:33+name_len].decode("ascii", errors="replace")
                
                # Strip revision if present (e.g. ;1)
                clean_name = name.split(";")[0]
                
                is_dir = (flags & 2) != 0
                if name not in ("\x00", "\x01"):
                    full_path = f"{current_path}/{clean_name}" if current_path else clean_name
                    if is_dir:
                        parse_dir(file_lba, file_size, full_path)
                    else:
                        raw_byte_offset = file_lba * sector_size + data_sub_offset
                        all_files.append((full_path, file_lba, file_size, raw_byte_offset))
                pos += rec_len

        parse_dir(root_lba, root_size)
        print(f"Found {len(all_files)} files on disc.")
        
        # Filter for models, enemies, stages, ARD, EMD, PLD
        interesting = [f for f in all_files if any(ext in f[0].upper() for ext in [".EMD", ".ARD", ".PLD", ".PLW", ".TIM", ".EDD", ".RDT", "STAGE", "ROOM", "BSS", "ZOM"])]
        print(f"\nFound {len(interesting)} enemy/stage/model-related files:")
        for path, lba, size, raw_off in interesting[:100]:
            print(f"  {path:35} | LBA: {lba:6} | Size: {size:8} | RawOff: {raw_off}")
            
        with open("disc_file_list.txt", "w", encoding="utf-8") as out:
            for path, lba, size, raw_off in all_files:
                out.write(f"{path}\t{lba}\t{size}\t{raw_off}\n")
        print("\nFull file list written to disc_file_list.txt")

if __name__ == "__main__":
    bin_path = r"Resident Evil 3 - dados\Resident Evil 3 - Nemesis (USA, Canada).bin"
    if not os.path.exists(bin_path):
        bin_path = r"ResidentEvil3-PCPort\gamedata\Resident Evil 3 - Nemesis (USA, Canada).bin"
    scan_iso(bin_path)
