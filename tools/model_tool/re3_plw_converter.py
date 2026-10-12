import os
import struct
import zlib

def parse_tim(data, offset):
    magic, bpp_flag = struct.unpack('<II', data[offset:offset+8])
    has_clut = bool(bpp_flag & 8)
    bpp_type = bpp_flag & 7
    pos = offset + 8
    clut_colors = []
    clut_info = {}
    if has_clut:
        clut_len, clut_x, clut_y, clut_w, clut_h = struct.unpack('<I4H', data[pos:pos+12])
        clut_bytes = data[pos+12:pos+clut_len]
        num_colors = clut_w * clut_h
        for c in range(num_colors):
            c16 = struct.unpack('<H', clut_bytes[c*2:c*2+2])[0]
            r = (c16 & 0x1F) << 3
            g = ((c16 >> 5) & 0x1F) << 3
            b = ((c16 >> 10) & 0x1F) << 3
            a = 0 if c16 == 0 else 255
            clut_colors.append((r, g, b, a))
        clut_info = {'x': clut_x, 'y': clut_y, 'w': clut_w, 'h': clut_h, 'raw': clut_bytes}
        pos += clut_len

    img_len, img_x, img_y, img_w, img_h = struct.unpack('<I4H', data[pos:pos+12])
    img_bytes = data[pos+12:pos+img_len]
    pix_w = img_w * 2 if bpp_type == 1 else (img_w * 4 if bpp_type == 0 else img_w)
    pix_h = img_h
    pixels = []
    for y in range(pix_h):
        row = []
        for x in range(pix_w):
            idx = img_bytes[y * pix_w + x]
            row.append(clut_colors[idx] if idx < len(clut_colors) else (0, 0, 0, 255))
        pixels.append(row)
    
    img_info = {'x': img_x, 'y': img_y, 'w': img_w, 'h': img_h, 'bpp': bpp_type}
    return pix_w, pix_h, pixels, clut_info, img_info

def save_png(w, h, pixels, out_path):
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        for x in range(w):
            r, g, b, a = pixels[y][x]
            raw.extend([r, g, b, a])
    compressed = zlib.compress(bytes(raw))

    def chunk(tag, data):
        c = bytearray(tag) + data
        crc = struct.pack('>I', zlib.crc32(c))
        return struct.pack('>I', len(data)) + c + crc

    png = bytearray(b'\x89PNG\r\n\x1a\n')
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', compressed)
    png += chunk(b'IEND', b'')

    with open(out_path, 'wb') as f:
        f.write(png)

def export_plw(plw_path, output_dir, model_name):
    os.makedirs(output_dir, exist_ok=True)
    with open(plw_path, 'rb') as f:
        data = f.read()

    dir_offset = struct.unpack('<I', data[:4])[0]
    count = struct.unpack('<I', data[dir_offset:dir_offset+4])[0]
    offsets = []
    for i in range(count):
        off = struct.unpack('<I', data[dir_offset+4+i*4:dir_offset+8+i*4])[0]
        offsets.append(off)

    sec7_start = offsets[6]
    sec7_end = offsets[7]
    sec7 = data[sec7_start:sec7_end]

    tim_offset = offsets[7]
    tex_w, tex_h, pixels, clut_info, img_info = parse_tim(data, tim_offset)
    tex_filename = f"{model_name}.png"
    tex_path = os.path.join(output_dir, tex_filename)
    save_png(tex_w, tex_h, pixels, tex_path)
    print(f"[{model_name}] Saved texture {tex_w}x{tex_h} to {tex_path}")

    # Section 7 Header:
    # 0..7: total_size, obj_count (1)
    # 8..31: obj_header (offsets relative to byte 8)
    total_sz, obj_count = struct.unpack('<2I', sec7[:8])
    norm_off, vert_off, vert_count, norm_count, prim_off = struct.unpack('<5I', sec7[8:28])
    tri_count, quad_count = struct.unpack('<HH', sec7[28:32])

    # Convert relative offsets to absolute in sec7:
    norm_start = 8 + norm_off
    vert_start = 8 + vert_off
    prim_start = 8 + prim_off

    print(f"[{model_name}] Mesh info: {vert_count} vertices, {norm_count} normals, {tri_count} triangles, {quad_count} quads")

    # Read Vertices
    vertices = []
    for i in range(vert_count):
        vx, vy, vz, _ = struct.unpack('<4h', sec7[vert_start + i*8 : vert_start + (i+1)*8])
        vertices.append((vx / 1000.0, -vy / 1000.0, vz / 1000.0))

    # Read Normals
    normals = []
    for i in range(norm_count):
        nx, ny, nz, _ = struct.unpack('<4h', sec7[norm_start + i*8 : norm_start + (i+1)*8])
        length = (nx*nx + ny*ny + nz*nz) ** 0.5
        if length > 0:
            normals.append((nx / length, -ny / length, nz / length))
        else:
            normals.append((0.0, 1.0, 0.0))

    # Read Primitives
    curr = prim_start
    triangles = []
    for _ in range(tri_count):
        tb = sec7[curr : curr + 12]
        curr += 12
        u0, v0 = tb[0], tb[1]
        clut = struct.unpack('<H', tb[2:4])[0]
        u1, v1 = tb[4], tb[5]
        page_lo = tb[6]
        v0_idx = tb[7]
        u2, v2 = tb[8], tb[9]
        v1_idx = tb[10]
        v2_idx = tb[11]
        triangles.append({
            'indices': [v0_idx, v1_idx, v2_idx],
            'uvs': [(u0, v0), (u1, v1), (u2, v2)],
            'clut': clut,
            'page_lo': page_lo,
            'raw': tb
        })

    quads = []
    for _ in range(quad_count):
        qb = sec7[curr : curr + 16]
        curr += 16
        u0, v0 = qb[0], qb[1]
        clut = struct.unpack('<H', qb[2:4])[0]
        u1, v1 = qb[4], qb[5]
        page = struct.unpack('<H', qb[6:8])[0]
        u2, v2 = qb[8], qb[9]
        v0_idx = qb[10]
        v1_idx = qb[11]
        u3, v3 = qb[12], qb[13]
        v2_idx = qb[14]
        v3_idx = qb[15]
        quads.append({
            'indices': [v0_idx, v1_idx, v2_idx, v3_idx],
            'uvs': [(u0, v0), (u1, v1), (u2, v2), (u3, v3)],
            'clut': clut,
            'page': page,
            'raw': qb
        })

    # Write MTL
    mtl_filename = f"{model_name}.mtl"
    mtl_path = os.path.join(output_dir, mtl_filename)
    with open(mtl_path, 'w') as f_mtl:
        f_mtl.write(f"# Material for {model_name}\n")
        f_mtl.write(f"newmtl weapon_mat\n")
        f_mtl.write("Ka 1.0 1.0 1.0\n")
        f_mtl.write("Kd 1.0 1.0 1.0\n")
        f_mtl.write("Ks 0.0 0.0 0.0\n")
        f_mtl.write(f"map_Kd {tex_filename}\n")

    # Write OBJ
    obj_filename = f"{model_name}.obj"
    obj_path = os.path.join(output_dir, obj_filename)
    with open(obj_path, 'w') as f_obj:
        f_obj.write(f"# RE3 3D Model: {model_name}\n")
        f_obj.write(f"mtllib {mtl_filename}\n")
        f_obj.write(f"o {model_name}\n\n")

        for vx, vy, vz in vertices:
            f_obj.write(f"v {vx:.6f} {vy:.6f} {vz:.6f}\n")
        f_obj.write("\n")

        for nx, ny, nz in normals:
            f_obj.write(f"vn {nx:.4f} {ny:.4f} {nz:.4f}\n")
        f_obj.write("\n")

        vt_list = []
        def get_vt_idx(u, v):
            norm_u = (u % tex_w) / float(tex_w)
            norm_v = 1.0 - ((v % tex_h) / float(tex_h))
            vt_list.append((norm_u, norm_v))
            return len(vt_list)

        f_obj.write("usemtl weapon_mat\n")
        f_obj.write("s 1\n")

        faces_str = []
        for tri in triangles:
            vt0 = get_vt_idx(*tri['uvs'][0])
            vt1 = get_vt_idx(*tri['uvs'][1])
            vt2 = get_vt_idx(*tri['uvs'][2])
            i0, i1, i2 = [i + 1 for i in tri['indices']]
            faces_str.append(f"f {i0}/{vt0}/{i0} {i1}/{vt1}/{i1} {i2}/{vt2}/{i2}\n")

        for quad in quads:
            vt0 = get_vt_idx(*quad['uvs'][0])
            vt1 = get_vt_idx(*quad['uvs'][1])
            vt2 = get_vt_idx(*quad['uvs'][2])
            vt3 = get_vt_idx(*quad['uvs'][3])
            i0, i1, i2, i3 = [i + 1 for i in quad['indices']]
            faces_str.append(f"f {i0}/{vt0}/{i0} {i1}/{vt1}/{i1} {i3}/{vt3}/{i3} {i2}/{vt2}/{i2}\n")

        for u, v in vt_list:
            f_obj.write(f"vt {u:.6f} {v:.6f}\n")
        f_obj.write("\n")

        for fs in faces_str:
            f_obj.write(fs)

    print(f"[{model_name}] Exported OBJ successfully to {obj_path}!")

def repack_plw_with_obj(template_plw_path, obj_path, out_plw_path):
    """
    Repacks a PLW file by injecting vertices and normals from an OBJ file.
    Preserves all animations, hand attachment, bone rigging and TIM texture.
    """
    with open(template_plw_path, 'rb') as f:
        plw_data = bytearray(f.read())

    dir_offset = struct.unpack('<I', plw_data[:4])[0]
    count = struct.unpack('<I', plw_data[dir_offset:dir_offset+4])[0]
    offsets = []
    for i in range(count):
        off = struct.unpack('<I', plw_data[dir_offset+4+i*4:dir_offset+8+i*4])[0]
        offsets.append(off)

    sec7_start = offsets[6]
    sec7_end = offsets[7]
    sec7 = plw_data[sec7_start:sec7_end]

    norm_off, vert_off, vert_count, norm_count, prim_off = struct.unpack('<5I', sec7[8:28])
    tri_count, quad_count = struct.unpack('<HH', sec7[28:32])

    vert_start = 8 + vert_off
    norm_start = 8 + norm_off

    # Parse OBJ vertices and normals
    obj_verts = []
    obj_norms = []
    with open(obj_path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if not parts: continue
            if parts[0] == 'v':
                x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                # Convert back to PS1 coordinates:
                vx = int(round(x * 1000.0))
                vy = int(round(-y * 1000.0))
                vz = int(round(z * 1000.0))
                obj_verts.append((vx, vy, vz))
            elif parts[0] == 'vn':
                nx, ny, nz = float(parts[1]), float(parts[2]), float(parts[3])
                # PS1 normal magnitude ~200
                obj_norms.append((int(round(nx * 200.0)), int(round(-ny * 200.0)), int(round(nz * 200.0))))

    print(f"Repacking: found {len(obj_verts)} vertices in OBJ (expected {vert_count})")
    
    # Update vertices in sec7
    for i in range(min(vert_count, len(obj_verts))):
        vx, vy, vz = obj_verts[i]
        struct.pack_into('<4h', sec7, vert_start + i*8, vx, vy, vz, 0)

    # Put sec7 back into plw_data
    plw_data[sec7_start:sec7_end] = sec7

    with open(out_plw_path, 'wb') as f:
        f.write(plw_data)

    print(f"Successfully repacked PLW to {out_plw_path}!")

if __name__ == '__main__':
    base_dir = os.path.dirname(os.path.abspath(__file__))
    knife_plw = os.path.join(base_dir, 'extracted_plw', 'PL00W00.PLW')
    export_plw(knife_plw, os.path.join(base_dir, 'models', 'knife'), 'knife')

    # Test roundtrip repacking
    repack_plw_with_obj(knife_plw, 
                        os.path.join(base_dir, 'models', 'knife', 'knife.obj'),
                        os.path.join(base_dir, 'models', 'knife', 'PL00W00_repacked.PLW'))

    handgun_plw = os.path.join(base_dir, 'extracted_plw', 'PL00W01.PLW')
    export_plw(handgun_plw, os.path.join(base_dir, 'models', 'handgun'), 'handgun')
