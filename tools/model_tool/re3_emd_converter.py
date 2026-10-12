import os
import struct
import zlib

class RE3EMDConverter:
    @staticmethod
    def parse_tim(data, offset):
        magic, bpp_flag = struct.unpack('<II', data[offset:offset+8])
        has_clut = bool(bpp_flag & 8)
        bpp_type = bpp_flag & 7
        pos = offset + 8
        palettes = []
        if has_clut:
            clut_len, clut_x, clut_y, clut_w, clut_h = struct.unpack('<I4H', data[pos:pos+12])
            pos += 12
            raw_colors = data[pos:pos + clut_w * clut_h * 2]
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
        
        img_len, img_x, img_y, img_w, img_h = struct.unpack('<I4H', data[pos:pos+12])
        pos += 12
        raw_img = data[pos:pos + img_w * img_h * 2]
        real_w = img_w * (2 if bpp_type == 1 else 4 if bpp_type == 0 else 1)
        
        return real_w, img_h, raw_img, img_w, palettes, bpp_type

    @staticmethod
    def save_png(w, h, pixels, out_path):
        raw = bytearray()
        for y in range(h):
            raw.append(0)
            for x in range(w):
                raw.extend(pixels[y][x])
        c = zlib.compress(bytes(raw))
        def chunk(tag, d):
            return struct.pack('>I', len(d)) + tag + d + struct.pack('>I', zlib.crc32(tag + d))
        with open(out_path, 'wb') as out_f:
            out_f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) + chunk(b'IDAT', c) + chunk(b'IEND', b''))

    @classmethod
    def export_emd(cls, emd_path, tim_path, output_dir, model_name):
        os.makedirs(output_dir, exist_ok=True)
        with open(emd_path, 'rb') as f:
            emd_data = f.read()
        with open(tim_path, 'rb') as f:
            tim_data = f.read()

        real_w, img_h, raw_img, img_w, palettes, bpp_type = cls.parse_tim(tim_data, 0)
        
        # Save textures
        for p_idx, pal in enumerate(palettes):
            pixels = []
            for y in range(img_h):
                row = []
                for x in range(real_w):
                    idx = raw_img[y * (img_w * 2) + x]
                    row.append(pal[idx] if idx < len(pal) else (0, 0, 0, 0))
                pixels.append(row)
            png_p = os.path.join(output_dir, f"{model_name}_pal{p_idx}.png")
            cls.save_png(real_w, img_h, pixels, png_p)
            
        main_png = f"{model_name}_pal0.png"
        
        # Parse EMD
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
            limb_headers.append({
                'v_off': v_off, 'n_off': n_off, 'v_cnt': v_cnt,
                'tri_off': tri_off, 'quad_off': quad_off,
                'tri_cnt': tri_cnt, 'quad_cnt': quad_cnt
            })
            hpos += 24

        obj_path = os.path.join(output_dir, f"{model_name}.obj")
        mtl_path = os.path.join(output_dir, f"{model_name}.mtl")
        
        with open(mtl_path, 'w') as mf:
            mf.write(f"newmtl Material_{model_name}\n")
            mf.write(f"map_Kd {main_png}\n")
            
        limb_names = [
            "pelvis_root", "spine_abdomen", "chest_torso", "head_neck",
            "left_upper_arm", "left_forearm", "left_hand",
            "right_upper_arm", "right_forearm", "right_hand",
            "left_thigh", "left_calf", "left_foot",
            "right_thigh", "right_calf"
        ]
        
        with open(obj_path, 'w') as of:
            of.write(f"# Resident Evil 3 EMD Export - {model_name}\n")
            of.write(f"mtllib {os.path.basename(mtl_path)}\n")
            of.write(f"usemtl Material_{model_name}\n\n")
            
            vert_base = 1
            uv_base = 1
            
            for l_idx, lh in enumerate(limb_headers):
                lname = limb_names[l_idx] if l_idx < len(limb_names) else f"limb_{l_idx:02d}"
                of.write(f"\ng {lname}\n")
                of.write(f"o {lname}\n")
                
                # Vertices
                for vi in range(lh['v_cnt']):
                    vx, vy, vz, _ = struct.unpack('<4h', sec14_data[8 + lh['v_off'] + vi*8 : 8 + lh['v_off'] + (vi+1)*8])
                    of.write(f"v {vx/1000.0:.4f} {-vy/1000.0:.4f} {vz/1000.0:.4f}\n")
                    
                # Triangles
                tri_uvs = []
                tri_faces = []
                for ti in range(lh['tri_cnt']):
                    tb = sec14_data[8 + lh['tri_off'] + ti*12 : 8 + lh['tri_off'] + (ti+1)*12]
                    u0, v0 = tb[0], tb[1]
                    u1, v1 = tb[4], tb[5]
                    v0_idx = tb[7]
                    u2, v2 = tb[8], tb[9]
                    v1_idx = tb[10]
                    v2_idx = tb[11]
                    tri_uvs.extend([
                        (u0 / real_w, (img_h - v0) / img_h),
                        (u1 / real_w, (img_h - v1) / img_h),
                        (u2 / real_w, (img_h - v2) / img_h)
                    ])
                    tri_faces.append((v0_idx, v1_idx, v2_idx))
                    
                # Quads
                quad_uvs = []
                quad_faces = []
                for qi in range(lh['quad_cnt']):
                    qb = sec14_data[8 + lh['quad_off'] + qi*16 : 8 + lh['quad_off'] + (qi+1)*16]
                    u0, v0 = qb[0], qb[1]
                    u1, v1 = qb[4], qb[5]
                    u2, v2 = qb[8], qb[9]
                    v0_idx = qb[10]
                    v1_idx = qb[11]
                    u3, v3 = qb[12], qb[13]
                    v2_idx = qb[14]
                    v3_idx = qb[15]
                    quad_uvs.extend([
                        (u0 / real_w, (img_h - v0) / img_h),
                        (u1 / real_w, (img_h - v1) / img_h),
                        (u2 / real_w, (img_h - v2) / img_h),
                        (u3 / real_w, (img_h - v3) / img_h)
                    ])
                    quad_faces.append((v0_idx, v1_idx, v2_idx, v3_idx))
                    
                # Write UVs
                for u, v in tri_uvs:
                    of.write(f"vt {u:.4f} {v:.4f}\n")
                for u, v in quad_uvs:
                    of.write(f"vt {u:.4f} {v:.4f}\n")
                    
                # Write faces
                cur_uv = uv_base
                for v0, v1, v2 in tri_faces:
                    of.write(f"f {vert_base+v0}/{cur_uv} {vert_base+v1}/{cur_uv+1} {vert_base+v2}/{cur_uv+2}\n")
                    cur_uv += 3
                for v0, v1, v2, v3 in quad_faces:
                    of.write(f"f {vert_base+v0}/{cur_uv} {vert_base+v1}/{cur_uv+1} {vert_base+v2}/{cur_uv+2} {vert_base+v3}/{cur_uv+3}\n")
                    cur_uv += 4
                    
                vert_base += lh['v_cnt']
                uv_base += len(tri_uvs) + len(quad_uvs)

        print(f"Exported {model_name} to {obj_path}")

    @classmethod
    def repack_emd(cls, original_emd_path, obj_path, out_emd_path):
        with open(original_emd_path, 'rb') as f:
            orig_data = f.read()

        dir_off = struct.unpack('<I', orig_data[:4])[0]
        table_bytes = orig_data[dir_off:]
        offsets = [struct.unpack('<I', table_bytes[i*4:i*4+4])[0] for i in range(len(table_bytes)//4)]
        sec14_orig_off = offsets[14]
        sec14_orig = orig_data[sec14_orig_off:dir_off]
        
        # Read original limb headers and primitives to preserve exact texture CLUT/pages
        sec_size, obj_count = struct.unpack('<II', sec14_orig[:8])
        orig_limb_headers = []
        hpos = 8
        for i in range(obj_count):
            v_off, n_off, v_cnt, tri_off, quad_off, tri_cnt, quad_cnt = struct.unpack('<5I2H', sec14_orig[hpos:hpos+24])
            orig_limb_headers.append({
                'v_off': v_off, 'n_off': n_off, 'v_cnt': v_cnt,
                'tri_off': tri_off, 'quad_off': quad_off,
                'tri_cnt': tri_cnt, 'quad_cnt': quad_cnt,
                'tris_data': sec14_orig[8 + tri_off : 8 + tri_off + tri_cnt * 12],
                'quads_data': sec14_orig[8 + quad_off : 8 + quad_off + quad_cnt * 16],
                'norms_data': sec14_orig[8 + n_off : 8 + n_off + v_cnt * 8]
            })
            hpos += 24

        # Parse OBJ file for new vertices
        limbs_verts = []
        current_verts = []
        with open(obj_path, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'): continue
                parts = line.split()
                if parts[0] in ('g', 'o'):
                    if current_verts:
                        limbs_verts.append(current_verts)
                        current_verts = []
                elif parts[0] == 'v':
                    vx = int(round(float(parts[1]) * 1000.0))
                    vy = int(round(-float(parts[2]) * 1000.0))
                    vz = int(round(float(parts[3]) * 1000.0))
                    current_verts.append((vx, vy, vz))
        if current_verts:
            limbs_verts.append(current_verts)

        # Build new Section 14
        # Stride: 8 bytes header + 15 * 24 headers
        # Then all limb primitives (tris + quads)
        # Then all limb vertices
        # Then all limb normals
        new_headers = []
        prims_buf = bytearray()
        
        # Calculate prim offsets
        cur_prim_off = 15 * 24
        for i in range(obj_count):
            lh = orig_limb_headers[i]
            tri_cnt = lh['tri_cnt']
            quad_cnt = lh['quad_cnt']
            t_off = cur_prim_off
            cur_prim_off += tri_cnt * 12
            q_off = cur_prim_off
            cur_prim_off += quad_cnt * 16
            
            prims_buf.extend(lh['tris_data'])
            prims_buf.extend(lh['quads_data'])
            
            new_headers.append({
                'tri_off': t_off,
                'quad_off': q_off,
                'tri_cnt': tri_cnt,
                'quad_cnt': quad_cnt,
                'v_cnt': len(limbs_verts[i]) if i < len(limbs_verts) else lh['v_cnt']
            })

        # Calculate vertex offsets
        verts_buf = bytearray()
        cur_vert_off = cur_prim_off
        for i in range(obj_count):
            new_headers[i]['v_off'] = cur_vert_off
            vlist = limbs_verts[i] if i < len(limbs_verts) else []
            for vx, vy, vz in vlist:
                verts_buf.extend(struct.pack('<4h', vx, vy, vz, 0))
            cur_vert_off += len(vlist) * 8

        # Calculate normal offsets
        norms_buf = bytearray()
        cur_norm_off = cur_vert_off
        for i in range(obj_count):
            new_headers[i]['n_off'] = cur_norm_off
            # Use original normals or recomputed
            lh = orig_limb_headers[i]
            v_cnt = new_headers[i]['v_cnt']
            if len(lh['norms_data']) == v_cnt * 8:
                norms_buf.extend(lh['norms_data'])
            else:
                # pad or generate normals
                for _ in range(v_cnt):
                    norms_buf.extend(struct.pack('<4h', 0, 4096, 0, 0))
            cur_norm_off += v_cnt * 8

        total_s14_size = 8 + cur_norm_off
        
        # Assemble Section 14
        s14_buf = bytearray()
        s14_buf.extend(struct.pack('<II', total_s14_size, obj_count))
        for nh in new_headers:
            s14_buf.extend(struct.pack('<5I2H',
                nh['v_off'], nh['n_off'], nh['v_cnt'],
                nh['tri_off'], nh['quad_off'],
                nh['tri_cnt'], nh['quad_cnt']
            ))
        s14_buf.extend(prims_buf)
        s14_buf.extend(verts_buf)
        s14_buf.extend(norms_buf)
        
        assert len(s14_buf) == total_s14_size, f"Size mismatch: {len(s14_buf)} vs {total_s14_size}"

        # Prefix is orig_data up to sec14_orig_off
        prefix = bytearray(orig_data[:sec14_orig_off])
        
        # New dir offset
        new_dir_off = len(prefix) + len(s14_buf)
        prefix[0:4] = struct.pack('<I', new_dir_off)
        
        # Directory table
        new_table = bytearray()
        for off in offsets[:14]:
            new_table.extend(struct.pack('<I', off))
        new_table.extend(struct.pack('<I', sec14_orig_off))
        
        repacked_emd = bytes(prefix + s14_buf + new_table)
        with open(out_emd_path, 'wb') as f:
            f.write(repacked_emd)
            
        return repacked_emd

if __name__ == "__main__":
    import hashlib
    # Roundtrip test on Zombie 54
    orig_path = r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.EMD'
    obj_path = r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54.obj'
    out_path = r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54_roundtrip.EMD'
    
    repacked = RE3EMDConverter.repack_emd(orig_path, obj_path, out_path)
    with open(orig_path, 'rb') as f:
        orig = f.read()
        
    print(f"Orig size: {len(orig)}, Repacked size: {len(repacked)}")
    orig_hash = hashlib.sha256(orig).hexdigest()
    rep_hash = hashlib.sha256(repacked).hexdigest()
    print(f"Orig SHA-256:     {orig_hash}")
    print(f"Repacked SHA-256: {rep_hash}")
    if orig_hash == rep_hash:
        print(">>> SUCCESS: 100% BIT-FOR-BIT IDENTICAL ROUNDTRIP! <<<")
    else:
        print("Differences found, checking...")
        diffs = [i for i in range(min(len(orig), len(repacked))) if orig[i] != repacked[i]]
        print(f"First diff at offset {diffs[0] if diffs else 'none'}")
