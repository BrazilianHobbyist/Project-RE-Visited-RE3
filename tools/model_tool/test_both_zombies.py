import os
import hashlib
from re3_emd_converter import RE3EMDConverter

def run_tests():
    # Extract orig_z59.EMD and orig_z59.TIM
    bin_path = r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN'
    with open(bin_path, 'rb') as f:
        f.seek(0x0AD000)
        tim_59 = f.read(66592)
        f.seek(0x0BD800)
        emd_59 = f.read(72512)
        
    with open(r'RE3Recomp\tools\model_tool\models\zombie_59\orig_z59.TIM', 'wb') as f:
        f.write(tim_59)
    with open(r'RE3Recomp\tools\model_tool\models\zombie_59\orig_z59.EMD', 'wb') as f:
        f.write(emd_59)
        
    with open(bin_path, 'rb') as f:
        f.seek(0x08A800)
        tim_54 = f.read(66592)
    with open(r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.TIM', 'wb') as f:
        f.write(tim_54)

    # 1. Export Zombie 54
    RE3EMDConverter.export_emd(
        r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.EMD',
        r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.TIM',
        r'RE3Recomp\tools\model_tool\models\zombie_54',
        'zombie_54'
    )
    
    # 2. Repack Zombie 54
    rep_54 = RE3EMDConverter.repack_emd(
        r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.EMD',
        r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54.obj',
        r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54_repacked.EMD'
    )
    with open(r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.EMD', 'rb') as f:
        orig_54 = f.read()
    h1 = hashlib.sha256(orig_54).hexdigest()
    h2 = hashlib.sha256(rep_54).hexdigest()
    print(f"Zombie 54 Orig:     {h1}")
    print(f"Zombie 54 Repacked: {h2}")
    if h1 == h2:
        print(">>> ZOMBIE 54: 100% BIT-FOR-BIT IDENTICAL ROUNDTRIP! <<<")
    else:
        diffs = [i for i in range(min(len(orig_54), len(rep_54))) if orig_54[i] != rep_54[i]]
        print(f"Diff count: {len(diffs)}, first diff at offset {diffs[0]}")

    # 3. Export Zombie 59
    RE3EMDConverter.export_emd(
        r'RE3Recomp\tools\model_tool\models\zombie_59\orig_z59.EMD',
        r'RE3Recomp\tools\model_tool\models\zombie_59\orig_z59.TIM',
        r'RE3Recomp\tools\model_tool\models\zombie_59',
        'zombie_59'
    )
    
    # 4. Repack Zombie 59
    rep_59 = RE3EMDConverter.repack_emd(
        r'RE3Recomp\tools\model_tool\models\zombie_59\orig_z59.EMD',
        r'RE3Recomp\tools\model_tool\models\zombie_59\zombie_59.obj',
        r'RE3Recomp\tools\model_tool\models\zombie_59\zombie_59_repacked.EMD'
    )
    with open(r'RE3Recomp\tools\model_tool\models\zombie_59\orig_z59.EMD', 'rb') as f:
        orig_59 = f.read()
    h3 = hashlib.sha256(orig_59).hexdigest()
    h4 = hashlib.sha256(rep_59).hexdigest()
    print(f"\nZombie 59 Orig:     {h3}")
    print(f"Zombie 59 Repacked: {h4}")
    if h3 == h4:
        print(">>> ZOMBIE 59: 100% BIT-FOR-BIT IDENTICAL ROUNDTRIP! <<<")
    else:
        diffs = [i for i in range(min(len(orig_59), len(rep_59))) if orig_59[i] != rep_59[i]]
        print(f"Diff count: {len(diffs)}, first diff at offset {diffs[0]}")

if __name__ == '__main__':
    run_tests()
