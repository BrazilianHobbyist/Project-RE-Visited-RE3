import os
import struct
import zlib

def export_emd_to_obj(bin_path, emd_offset, emd_size, tim_offset, tim_size, out_prefix, model_name):
    os.makedirs(os.path.dirname(out_prefix), exist_ok=True)
    with open(bin_path, 'rb') as f:
        f.seek(emd_offset)
        emd_data = f.read(emd_size)
        f.seek(tim_offset)
        tim_data = f.read(tim_size)

    # 1. Parse TIM texture & export PNG for all palettes
    tag, bpp = struct.unpack('<II', tim_data[:8])
    pos = 8
    has_clut = (bpp & 8) != 0
    mode = bpp & 7
    palettes = []
    if has_clut:
        clut_len, clut_x, clut_y, clut_w, clut_h = struct.unpack('<I4H', tim_data[pos:pos+12])
        pos += 12
        raw_colors = tim_data[pos:pos + clut_w * clut_h * 2]
        pos += clut_w * clut_h * 2
        for pal_idx in range(clut_h):
            pal = []
            for c_idx in range(clut_w):
                c_off = (pal_idx * clut_w + c_idx) * 2
                c_val = struct.unpack('<H', raw_colors[c_off:c_off+2])[0]
                r = (c_val & 0x1F) << 3
                g = ((c_val >> 5) & 0x1F) << 3
                b = ((c_val >> 10) & 0x1F) << 3
                a = 0 if c_val == 0 else 255
                pal.append((r, g, b, a))
            palettes.append(pal)
            
    img_len, img_x, img_y, img_w, img_h = struct.unpack('<I4H', tim_data[pos:pos+12])
    pos += 12
    raw_img = tim_data[pos:pos + img_w * img_h * 2]
    real_w = img_w * (2 if mode == 1 else 4 if mode == 0 else 1)
    
    def save_png(w, h, pixels, out_p):
        raw = bytearray()
        for y in range(h):
            raw.append(0)
            for x in range(w):
                raw.extend(pixels[y][x])
        c = zlib.compress(bytes(raw))
        def chunk(tag, d):
            return struct.pack('>I', len(d)) + tag + d + struct.pack('>I', zlib.crc32(tag + d))
        with open(out_p, 'wb') as out_f:
            out_f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) + chunk(b'IDAT', c) + chunk(b'IEND', b''))

    for p_idx, pal in enumerate(palettes):
        pixels = []
        for y in range(img_h):
            row = []
            for x in range(real_w):
                idx = raw_img[y * (img_w * 2) + x]
                row.append(pal[idx] if idx < len(pal) else (0, 0, 0, 0))
            pixels.append(row)
        png_path = f"{out_prefix}_pal{p_idx}.png"
        save_png(real_w, img_h, pixels, png_path)
        print(f"  Saved texture palette {p_idx}: {png_path}")

    # Primary texture used for MTL
    main_png_path = f"{out_prefix}_pal0.png"

    # 2. Parse Section 14 (Mesh)
    dir_off = struct.unpack('<I', emd_data[:4])[0]
    table_bytes = emd_data[dir_off:]
    offsets = [struct.unpack('<I', table_bytes[i*4:i*4+4])[0] for i in range(len(table_bytes)//4)]
    sec14_off = offsets[14]
    sec14_data = emd_data[sec14_off:dir_off]
    
    sec_size, obj_count = struct.unpack('<II', sec14_data[:8])
    
    limb_headers = []
    hpos = 8
    for i in range(obj_count):
        v_off, n_off, v_cnt, tri_off, quad_off, tri_cnt, quad_cnt = struct.unpack('<5I2H', sec14_data[hpos:hpos+24])
        limb_headers.append((v_off, n_off, v_cnt, tri_off, quad_off, tri_cnt, quad_cnt))
        hpos += 24
        
    obj_path = f"{out_prefix}.obj"
    mtl_path = f"{out_prefix}.mtl"
    
    with open(mtl_path, 'w') as mf:
        mf.write(f"newmtl Material_{model_name}\n")
        mf.write(f"map_Kd {os.path.basename(main_png_path)}\n")
        
    with open(obj_path, 'w') as of:
        of.write(f"# Resident Evil 3 EMD Export - {model_name}\n")
        of.write(f"mtllib {os.path.basename(mtl_path)}\n")
        of.write(f"usemtl Material_{model_name}\n\n")
        
        vert_global_offset = 1
        uv_global_offset = 1
        
        total_verts = 0
        total_tris = 0
        total_quads = 0
        
        limb_names = [
            "pelvis_root", "spine_abdomen", "chest_torso", "head_neck",
            "left_upper_arm", "left_forearm", "left_hand",
            "right_upper_arm", "right_forearm", "right_hand",
            "left_thigh", "left_calf", "left_foot",
            "right_thigh", "right_calf"
        ]
        
        for limb_idx, (v_off, n_off, v_cnt, tri_off, quad_off, tri_cnt, quad_cnt) in enumerate(limb_headers):
            lname = limb_names[limb_idx] if limb_idx < len(limb_names) else f"limb_{limb_idx:02d}"
            of.write(f"\ng {lname}\n")
            of.write(f"o {lname}\n")
            
            # Vertices
            for vi in range(v_cnt):
                vx, vy, vz, _ = struct.unpack('<4h', sec14_data[v_off + vi*8 : v_off + vi*8 + 8])
                # Scale by 1/1000 for standard 3D meter scale
                of.write(f"v {vx/1000.0:.4f} {-vy/1000.0:.4f} {vz/1000.0:.4f}\n")
                
            # Triangles
            tri_uv_list = []
            tri_face_indices = []
            for ti in range(tri_cnt):
                tb = sec14_data[tri_off + ti*12 : tri_off + ti*12 + 12]
                u0, v0 = tb[0], tb[1]
                u1, v1 = tb[4], tb[5]
                v0_idx = tb[7]
                u2, v2 = tb[8], tb[9]
                v1_idx = tb[10]
                v2_idx = tb[11]
                
                tri_uv_list.extend([
                    (u0 / real_w, (img_h - v0) / img_h),
                    (u1 / real_w, (img_h - v1) / img_h),
                    (u2 / real_w, (img_h - v2) / img_h)
                ])
                tri_face_indices.append((v0_idx, v1_idx, v2_idx))
                
            # Quads
            quad_uv_list = []
            quad_face_indices = []
            for qi in range(quad_cnt):
                qb = sec14_data[quad_off + qi*16 : quad_off + qi*16 + 16]
                u0, v0 = qb[0], qb[1]
                u1, v1 = qb[4], qb[5]
                u2, v2 = qb[8], qb[9]
                v0_idx = qb[10]
                v1_idx = qb[11]
                u3, v3 = qb[12], qb[13]
                v2_idx = qb[14]
                v3_idx = qb[15]
                
                quad_uv_list.extend([
                    (u0 / real_w, (img_h - v0) / img_h),
                    (u1 / real_w, (img_h - v1) / img_h),
                    (u2 / real_w, (img_h - v2) / img_h),
                    (u3 / real_w, (img_h - v3) / img_h)
                ])
                quad_face_indices.append((v0_idx, v1_idx, v2_idx, v3_idx))
                
            # Write UVs
            for uv in tri_uv_list:
                of.write(f"vt {uv[0]:.4f} {uv[1]:.4f}\n")
            for uv in quad_uv_list:
                of.write(f"vt {uv[0]:.4f} {uv[1]:.4f}\n")
                
            # Write Faces
            cur_uv = uv_global_offset
            for v0_idx, v1_idx, v2_idx in tri_face_indices:
                f0 = vert_global_offset + v0_idx
                f1 = vert_global_offset + v1_idx
                f2 = vert_global_offset + v2_idx
                of.write(f"f {f0}/{cur_uv} {f1}/{cur_uv+1} {f2}/{cur_uv+2}\n")
                cur_uv += 3
                
            for v0_idx, v1_idx, v2_idx, v3_idx in quad_face_indices:
                f0 = vert_global_offset + v0_idx
                f1 = vert_global_offset + v1_idx
                f2 = vert_global_offset + v2_idx
                f3 = vert_global_offset + v3_idx
                of.write(f"f {f0}/{cur_uv} {f1}/{cur_uv+1} {f2}/{cur_uv+2} {f3}/{cur_uv+3}\n")
                cur_uv += 4
                
            vert_global_offset += v_cnt
            uv_global_offset += len(tri_uv_list) + len(quad_uv_list)
            total_verts += v_cnt
            total_tris += tri_cnt
            total_quads += quad_cnt

    print(f"Successfully exported {model_name}: {total_verts} vertices, {total_tris} tris, {total_quads} quads -> {obj_path}")

if __name__ == "__main__":
    bin_path = r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN'
    export_emd_to_obj(bin_path, 0x09B000, 72724, 0x08A800, 66592, r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54', "Zombie_Male_54")
    export_emd_to_obj(bin_path, 0x0BD800, 72512, 0x0AD000, 66592, r'RE3Recomp\tools\model_tool\models\zombie_59\zombie_59', "Zombie_Female_59")
