import struct

def inspect_file_structure(path, offset, size):
    with open(path, 'rb') as f:
        f.seek(offset)
        data = f.read(size)
    print(f"=== Inspecting (offset 0x{offset:X}, size {size}) ===")
    w0, w1 = struct.unpack('<II', data[:8])
    print(f"Header: w0=0x{w0:X} ({w0}), w1=0x{w1:X} ({w1})")
    
    # Check at w0
    if w0 < len(data):
        print(f"Data at w0 (0x{w0:X}):")
        for i in range(w0, min(w0 + 64, len(data)), 16):
            chunk = data[i:i+16]
            print(f"  {i:04X}: {' '.join(f'{b:02X}' for b in chunk)}")
            
    print("End of file (last 64 bytes):")
    for i in range(max(0, len(data)-64), len(data), 16):
        chunk = data[i:i+16]
        print(f"  {i:04X}: {' '.join(f'{b:02X}' for b in chunk)}")

if __name__ == "__main__":
    inspect_file_structure(r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN', 0x09B000, 72724)
