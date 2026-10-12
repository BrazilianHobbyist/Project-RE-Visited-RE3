import struct

def parse_emd_mesh_section(path, offset, size):
    with open(path, 'rb') as f:
        f.seek(offset)
        data = f.read(size)
        
    size_field, obj_count = struct.unpack('<II', data[:8])
    print(f"Mesh Section: size={size_field}, obj_count={obj_count}")
    
    headers = []
    pos = 8
    for i in range(obj_count):
        # 24 bytes: 5 uint32 + 2 uint16
        w0, w1, w2, w3, w4, tri_cnt, quad_cnt = struct.unpack('<5I2H', data[pos:pos+24])
        headers.append((w0, w1, w2, w3, w4, tri_cnt, quad_cnt))
        pos += 24
        print(f"Limb {i:2d}: w0=0x{w0:X}, w1=0x{w1:X}, w2=0x{w2:X}, w3=0x{w3:X}, w4=0x{w4:X} | tris={tri_cnt:3d}, quads={quad_cnt:3d}")
        
    # Let's inspect w0, w1, w2, w3, w4 for Limb 0
    # In PLW, vertex was 8 bytes (int16 x, y, z, pad=0), normal was 8 bytes (int16 nx, ny, nz, pad=0)
    # Triangle was 12 bytes, Quad was 16 bytes
    # Let's check which of w0..w4 is vert_offset, norm_offset, prim_offset!
    for i in range(min(3, obj_count)):
        w0, w1, w2, w3, w4, tri_cnt, quad_cnt = headers[i]
        print(f"\n--- Limb {i} offsets investigation ---")
        for name, val in [("w0", w0), ("w1", w1), ("w3", w3), ("w4", w4)]:
            if val < len(data):
                # check if it looks like vertices (int16)
                sample = data[val:val+16]
                v_sample = struct.unpack('<4h', sample[:8])
                print(f"  {name} (0x{val:X}): sample hex={sample.hex()} as int16={v_sample}")

if __name__ == "__main__":
    with open(r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN', 'rb') as f:
        bin_data = f.read()
    # Entry 9 mesh section is at offset 0xE730 within Entry 9
    mesh_data = bin_data[0x09B000 + 0xE730 : 0x09B000 + 0xE730 + 13480]
    parse_emd_mesh_section(r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN', 0x09B000 + 0xE730, 13480)
