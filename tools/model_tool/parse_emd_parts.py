import struct

def parse_emd_parts(path, offset, size):
    with open(path, 'rb') as f:
        f.seek(offset)
        data = f.read(size)
        
    dir_offset, count = struct.unpack('<II', data[:8])
    print(f"EMD: dir_offset=0x{dir_offset:X}, bone_count={count}")
    
    # Read the 15 offsets from dir_offset
    part_offsets = []
    num_entries = (len(data) - dir_offset) // 4
    for i in range(num_entries):
        off = struct.unpack('<I', data[dir_offset + i*4 : dir_offset + i*4 + 4])[0]
        part_offsets.append(off)
        
    print(f"Directory table has {len(part_offsets)} entries:")
    for idx, poff in enumerate(part_offsets):
        # Inspect each part header
        # In PLW, section 7 had: size (4 bytes), obj_count (4 bytes), then 8 relative offsets:
        # norm_off, vert_off, vert_cnt, norm_cnt, prim_off, tri_cnt, quad_cnt
        p_len = struct.unpack('<I', data[poff:poff+4])[0] if poff + 4 <= len(data) else 0
        obj_c = struct.unpack('<I', data[poff+4:poff+8])[0] if poff + 8 <= len(data) else 0
        if poff + 22 <= len(data):
            n_off, v_off, v_cnt, n_cnt, pr_off, tri_cnt, quad_cnt = struct.unpack('<4H3H', data[poff+8:poff+22])
            print(f"  Bone {idx:2d} at 0x{poff:04X} | size={p_len:5d} | verts={v_cnt:3d}, norms={n_cnt:3d}, tris={tri_cnt:3d}, quads={quad_cnt:3d}")
        else:
            print(f"  Bone {idx:2d} at 0x{poff:04X} | size={p_len:5d} (short)")

if __name__ == "__main__":
    parse_emd_parts(r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN', 0x09B000, 72724)
