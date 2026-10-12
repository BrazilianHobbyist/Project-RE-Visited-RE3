import struct

def analyze_emd(data, name):
    print(f"=== Analyzing EMD: {name} (size {len(data)}) ===")
    for i in range(0, min(len(data), 64), 16):
        chunk = data[i:i+16]
        hex_s = ' '.join(f'{b:02X}' for b in chunk)
        print(f"  {i:04X}: {hex_s}")
        
    # Check directory / offsets
    # Word 0: total size or dir offset
    w0, w1 = struct.unpack('<II', data[:8])
    print(f"  Word 0: 0x{w0:X} ({w0}), Word 1: 0x{w1:X} ({w1})")
    
    # If w1 == 15 (0x0F), let's check if the next words are relative offsets to the 15 parts!
    if w1 == 15 or (w1 & 0xFFFF) == 15:
        count = w1 & 0xFFFF
        print(f"  Object count: {count}")
        # Let's inspect the next entries
        offsets = []
        for j in range(count):
            off = struct.unpack('<I', data[8+j*4:8+j*4+4])[0]
            offsets.append(off)
            print(f"    Part {j:2d}: offset=0x{off:X} ({off})")
            
        # Inspect part 0 header
        if count > 0 and offsets[0] < len(data):
            p0 = offsets[0]
            print(f"  Part 0 data at 0x{p0:X}:")
            for i in range(p0, min(p0 + 32, len(data)), 16):
                chunk = data[i:i+16]
                print(f"    {i:04X}: {' '.join(f'{b:02X}' for b in chunk)}")
                
            # Compare with PLW section 7 mesh header!
            # In PLW, byte 8 relative: norm_offset, vert_offset, vert_count, norm_count, prim_offset, tri_count, quad_count
            # Let's check if each part in EMD has the exact same section 7 structure!
            if p0 + 28 <= len(data):
                n_off, v_off, v_cnt, n_cnt, pr_off, tri_cnt, quad_cnt = struct.unpack('<4H3H', data[p0+8:p0+22])
                print(f"    Decoded mesh header of Part 0:")
                print(f"      norm_off=0x{n_off:X}, vert_off=0x{v_off:X}, verts={v_cnt}, norms={n_cnt}, prim_off=0x{pr_off:X}, tris={tri_cnt}, quads={quad_cnt}")

if __name__ == "__main__":
    with open(r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN', 'rb') as f:
        bin_data = f.read()
    analyze_emd(bin_data[0x067000:0x067000+144964], "Entry 7 (Enemy 0x10)")
    analyze_emd(bin_data[0x09B000:0x09B000+72724], "Entry 9 (Enemy 0x54)")
    analyze_emd(bin_data[0x0BD800:0x0BD800+72512], "Entry 11 (Enemy 0x59)")
