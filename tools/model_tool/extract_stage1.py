import os
import struct

def extract_files(file_list, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    bin_path = r"Resident Evil 3 - dados\Resident Evil 3 - Nemesis (USA, Canada).bin"
    if not os.path.exists(bin_path):
        bin_path = r"ResidentEvil3-PCPort\gamedata\Resident Evil 3 - Nemesis (USA, Canada).bin"
        
    txt_path = "disc_file_list.txt" if os.path.exists("disc_file_list.txt") else os.path.join(os.path.dirname(__file__), "disc_file_list.txt")
    with open(txt_path, "r", encoding="utf-8") as f:
        all_files = {}
        for line in f:
            parts = line.strip().split("\t")
            if len(parts) == 4:
                all_files[parts[0]] = (int(parts[1]), int(parts[2]), int(parts[3]))

    with open(bin_path, "rb") as bin_f:
        for fname in file_list:
            if fname in all_files:
                lba, size, raw_off = all_files[fname]
                print(f"Extracting {fname} (Size: {size}, LBA: {lba})...")
                
                # Read sector by sector (Mode 2 Form 1, 2352 bytes per sector, data starts at +24)
                data = bytearray()
                sec_count = (size + 2047) // 2048
                for i in range(sec_count):
                    bin_f.seek((lba + i) * 2352 + 24)
                    data.extend(bin_f.read(2048))
                
                content = bytes(data[:size])
                base_name = os.path.basename(fname)
                out_path = os.path.join(out_dir, base_name)
                with open(out_path, "wb") as out_f:
                    out_f.write(content)
                print(f"  -> Saved to {out_path} ({len(content)} bytes)")
            else:
                print(f"File {fname} not found on disc list!")

if __name__ == "__main__":
    to_extract = [
        "CD_DATA/STAGE1/ESTAGE1.BIN",
        "CD_DATA/STAGE1/R100.ARD",
        "CD_DATA/STAGE1/R101.ARD",
        "CD_DATA/STAGE1/R102.ARD",
        "CD_DATA/STAGE1/R101.BIN",
        "CD_DATA/STAGE1/R101.BSS"
    ]
    extract_files(to_extract, r"RE3Recomp\tools\model_tool\extracted_stage1")
