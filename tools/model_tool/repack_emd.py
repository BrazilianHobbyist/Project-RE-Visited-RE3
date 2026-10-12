import os
import struct

def repack_emd(original_emd_path, obj_path, out_emd_path):
    # Read original EMD to preserve Sections 0-13 (animations, skeleton data)
    with open(original_emd_path, 'rb') as f:
        orig_data = f.read()

    dir_off = struct.unpack('<I', orig_data[:4])[0]
    table_bytes = orig_data[dir_off:]
    offsets = [struct.unpack('<I', table_bytes[i*4:i*4+4])[0] for i in range(len(table_bytes)//4)]
    sec14_orig_off = offsets[14]
    
    # Sections 0-13 stay exactly as original
    preserved_prefix = orig_data[:sec14_orig_off]
    
    # Parse OBJ file with the 15 limb groups
    # Groups: pelvis_root, spine_abdomen, etc.
    limbs = []
    current_limb = None
    with open(obj_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'): continue
            parts = line.split()
            if parts[0] == 'g' or parts[0] == 'o':
                gname = parts[1]
                # Start new limb if not already matching current
                if not current_limb or current_limb['name'] != gname:
                    current_limb = {
                        'name': gname,
                        'verts': [],
                        'tris': [],
                        'quads': []
                    }
                    limbs.append(current_limb)
            elif parts[0] == 'v':
                # Convert back from meters to PS1 int16 coordinates
                vx = int(round(float(parts[1]) * 1000.0))
                vy = int(round(-float(parts[2]) * 1000.0))
                vz = int(round(float(parts[3]) * 1000.0))
                if current_limb:
                    current_limb['verts'].append((vx, vy, vz))
            elif parts[0] == 'f':
                # Parse face indices (relative to limb)
                face_verts = []
                for p in parts[1:]:
                    v_idx = int(p.split('/')[0])
                    face_verts.append(v_idx)
                if len(face_verts) == 3:
                    if current_limb: current_limb['tris'].append(face_verts)
                elif len(face_verts) == 4:
                    if current_limb: current_limb['quads'].append(face_verts)

    print(f"Parsed {len(limbs)} limbs from {obj_path}")

    # Also read original Section 14 to preserve UV/CLUT/page primitive data if needed,
    # or reconstruct Section 14
    sec14_orig = orig_data[sec14_orig_off:dir_off]
    sec_size, obj_count = struct.unpack('<II', sec14_orig[:8])
    
    # Return verification summary
    return len(preserved_prefix), len(sec14_orig), obj_count

if __name__ == "__main__":
    orig_bin = r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN'
    with open(orig_bin, 'rb') as f:
        f.seek(0x09B000)
        z54_emd = f.read(72724)
        
    with open(r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.EMD', 'wb') as f:
        f.write(z54_emd)
        
    pre_len, s14_len, obj_cnt = repack_emd(
        r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.EMD',
        r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54.obj',
        r'RE3Recomp\tools\model_tool\models\zombie_54\repacked_z54.EMD'
    )
    print(f"Prefix: {pre_len} bytes, Section 14: {s14_len} bytes, Bone count: {obj_cnt}")
