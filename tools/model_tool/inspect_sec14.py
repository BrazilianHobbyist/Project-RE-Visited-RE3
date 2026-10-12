import struct

def inspect_section_14():
    with open(r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN', 'rb') as f:
        f.seek(0x09B000 + 0xE730)
        data = f.read(13480)
        
    size, obj_count = struct.unpack('<II', data[:8])
    print(f"Section 14: size={size}, obj_count={obj_count}")
    
    # Let's inspect the headers for the 15 objects
    # Let's see if there are 15 offsets, or 15 header structs of 14/16 bytes
    pos = 8
    # Let's print the first 256 bytes
    for i in range(0, 256, 16):
        c = data[i:i+16]
        print(f"  {i:04X}: {' '.join(f'{b:02X}' for b in c)}")

if __name__ == "__main__":
    inspect_section_14()
