import os
import struct
import math
from re3_emd_converter import RE3EMDConverter

def enhance_zombie_male(orig_obj_path, custom_obj_path):
    print("Generating enhanced Male Zombie 3D model...")
    with open(orig_obj_path, 'r') as f:
        lines = f.readlines()
        
    out_lines = []
    current_limb = None
    
    for line in lines:
        if line.startswith('g ') or line.startswith('o '):
            current_limb = line.strip().split()[1]
            out_lines.append(line)
        elif line.startswith('v '):
            parts = [float(p) for p in line.strip().split()[1:4]]
            x, y, z = parts[0], parts[1], parts[2]
            
            # Subtly refine geometry per limb:
            if current_limb == 'head_neck':
                # Refine facial projection and jaw angle
                if z > 0:
                    z *= 1.08
                if y > 0:
                    y *= 1.04
            elif current_limb == 'chest_torso':
                # Beef up deltoids / upper torso breadth (+8% X breadth for broader decayed shoulders)
                if abs(x) > 0.08:
                    x *= 1.08
                # Chest depth
                if z > 0:
                    z *= 1.06
            elif current_limb == 'spine_abdomen':
                # Taper waist slightly for classic zombie hunch silhouette
                if abs(x) > 0.06:
                    x *= 0.98
            elif current_limb in ('left_upper_arm', 'right_upper_arm'):
                # Strengthen shoulder muscle mass
                if abs(x) > 0.05:
                    x *= 1.06
            elif current_limb in ('left_forearm', 'right_forearm'):
                # Accentuate torn sleeve contour and forearm reach
                z *= 1.04
            elif current_limb in ('left_thigh', 'right_thigh', 'left_calf', 'right_calf'):
                # Muscular calf contour
                if abs(x) > 0.08:
                    x *= 1.05
                    
            out_lines.append(f"v {x:.4f} {y:.4f} {z:.4f}\n")
        else:
            out_lines.append(line)
            
    with open(custom_obj_path, 'w') as f:
        f.writelines(out_lines)
    print(f"Saved custom Male Zombie to {custom_obj_path}")

def enhance_zombie_female(orig_obj_path, custom_obj_path):
    print("Generating enhanced Female Zombie 3D model...")
    with open(orig_obj_path, 'r') as f:
        lines = f.readlines()
        
    out_lines = []
    current_limb = None
    
    for line in lines:
        if line.startswith('g ') or line.startswith('o '):
            current_limb = line.strip().split()[1]
            out_lines.append(line)
        elif line.startswith('v '):
            parts = [float(p) for p in line.strip().split()[1:4]]
            x, y, z = parts[0], parts[1], parts[2]
            
            # Subtly refine female geometry per limb:
            if current_limb == 'head_neck':
                # Slender feminine jaw and defined chin profile
                if abs(x) > 0.05:
                    x *= 0.96
                if z > 0:
                    z *= 1.05
            elif current_limb == 'chest_torso':
                # Refined feminine torso and collarbone definition
                if z > 0:
                    z *= 1.08
            elif current_limb == 'spine_abdomen':
                # Slender feminine waist contour (-6% X)
                if abs(x) > 0.04:
                    x *= 0.94
            elif current_limb == 'pelvis_root':
                # Feminine hip/skirt silhouette
                if abs(x) > 0.06:
                    x *= 1.05
            elif current_limb in ('left_upper_arm', 'right_upper_arm', 'left_forearm', 'right_forearm'):
                # Slender feminine arm proportions
                if abs(x) > 0.04:
                    x *= 0.95
            elif current_limb in ('left_calf', 'right_calf'):
                # Defined feminine calf and ankle silhouette
                if abs(x) > 0.06:
                    x *= 0.96
                    
            out_lines.append(f"v {x:.4f} {y:.4f} {z:.4f}\n")
        else:
            out_lines.append(line)
            
    with open(custom_obj_path, 'w') as f:
        f.writelines(out_lines)
    print(f"Saved custom Female Zombie to {custom_obj_path}")

def build_custom_zombies():
    z54_orig = r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54.obj'
    z54_cust = r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54_custom.obj'
    enhance_zombie_male(z54_orig, z54_cust)
    
    z59_orig = r'RE3Recomp\tools\model_tool\models\zombie_59\zombie_59.obj'
    z59_cust = r'RE3Recomp\tools\model_tool\models\zombie_59\zombie_59_custom.obj'
    enhance_zombie_female(z59_orig, z59_cust)
    
    # Repack both into custom EMDs
    emd54_orig = r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.EMD'
    emd54_cust = r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54_custom.EMD'
    RE3EMDConverter.repack_emd(emd54_orig, z54_cust, emd54_cust)
    print(f"Repacked Male Zombie EMD -> {emd54_cust} ({os.path.getsize(emd54_cust)} bytes)")
    
    emd59_orig = r'RE3Recomp\tools\model_tool\models\zombie_59\orig_z59.EMD'
    emd59_cust = r'RE3Recomp\tools\model_tool\models\zombie_59\zombie_59_custom.EMD'
    RE3EMDConverter.repack_emd(emd59_orig, z59_cust, emd59_cust)
    print(f"Repacked Female Zombie EMD -> {emd59_cust} ({os.path.getsize(emd59_cust)} bytes)")

if __name__ == '__main__':
    build_custom_zombies()
