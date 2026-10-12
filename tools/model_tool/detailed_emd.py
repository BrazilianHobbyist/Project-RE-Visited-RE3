import struct

def detailed_emd_inspect():
    with open(r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN', 'rb') as f:
        f.seek(0x09B000)
        data = f.read(72724)
        
    dir_off, count = struct.unpack('<II', data[:8])
    print(f"Header: dir_off=0x{dir_off:X}, count={count}")
    
    # Read the directory table
    table_bytes = data[dir_off:]
    offsets = [struct.unpack('<I', table_bytes[i*4:i*4+4])[0] for i in range(len(table_bytes)//4)]
    print(f"Directory table (len {len(offsets)}):")
    for i, off in enumerate(offsets):
        print(f"  [{i:2d}] offset=0x{off:04X} ({off:5d})")
        
    # Let's inspect what is at each offset
    for i in range(len(offsets)):
        start = offsets[i]
        end = offsets[i+1] if i + 1 < len(offsets) else dir_off
        chunk = data[start:end]
        print(f"\n--- Section {i} (0x{start:04X} to 0x{end:04X}, len={len(chunk)}) ---")
        for r in range(0, min(len(chunk), 48), 16):
            c = chunk[r:r+16]
            print(f"  {r:04X}: {' '.join(f'{b:02X}' for b in c)}")

if __name__ == "__main__":
    detailed_emd_inspect()
