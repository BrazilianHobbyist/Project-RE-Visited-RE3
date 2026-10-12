import os
import shutil
import hashlib

def main():
    mod_root = r'ResidentEvil3-PCPort\mods\installed\re3.weapons.custom\1.0.0'
    assets_dir = os.path.join(mod_root, 'assets')
    os.makedirs(assets_dir, exist_ok=True)

    # 1. Copy custom PLWs
    knife_src = r'RE3Recomp\tools\model_tool\models\knife\PL00W00_custom.PLW'
    knife_dst = os.path.join(assets_dir, 'PL00W00.PLW')
    shutil.copy2(knife_src, knife_dst)

    hg_src = r'RE3Recomp\tools\model_tool\models\handgun\PL00W01_custom.PLW'
    hg_dst = os.path.join(assets_dir, 'PL00W01.PLW')
    shutil.copy2(hg_src, hg_dst)

    with open(knife_dst, 'rb') as f:
        knife_sha = hashlib.sha256(f.read()).hexdigest()
    with open(hg_dst, 'rb') as f:
        hg_sha = hashlib.sha256(f.read()).hexdigest()

    print(f"Knife SHA256: {knife_sha}")
    print(f"Handgun SHA256: {hg_sha}")

    # Manifest
    manifest_lines = [
        'format_version = 7',
        'id = "re3.weapons.custom"',
        'version = "1.0.0"',
        'name = "Custom Weapons 3D Pack"',
        'author = "Dev & Antigravity"',
        'description = "Modelos 3D personalizados para a Faca e a Pistola M92F."',
        'license = "MIT"',
        'resolver = "declarative"',
        '',
        '[[target]]',
        'game_id = "SLUS-00923"',
        'disc_sha256 = "d6c828dc92d50b28fd58329dbe607d4d22eeaeb1d9a46be54ac81b130fc2c7a6"',
        '',
        '[[target]]',
        'game_id = "SLUS-00923"',
        'disc_sha256 = "566c23468bf55f4877b14d12389e13cb6140824759f4deb46d69be33ad02d1db"',
        '',
        '[[feature]]',
        'id = "tactical-knife"',
        'name = "Faca de Combate Tatica"',
        'description = "Substitui a faca padrao por uma lamina tatica estendida."',
        'default_enabled = true',
        '',
        '[[feature]]',
        'id = "tactical-handgun"',
        'name = "Pistola Tatica Samurai Edge"',
        'description = "Substitui a M92F por uma versao tatica com compensador estendido."',
        'default_enabled = true',
        '',
        '[[overlay]]',
        'feature = "tactical-knife"',
        'target = "disc_user"',
        'offset = 26372096',
        'file = "assets/PL00W00.PLW"',
        f'sha256 = "{knife_sha}"',
        '',
        '[[overlay]]',
        'feature = "tactical-handgun"',
        'target = "disc_user"',
        'offset = 26417152',
        'file = "assets/PL00W01.PLW"',
        f'sha256 = "{hg_sha}"',
        ''
    ]

    manifest_path = os.path.join(mod_root, 'manifest.toml')
    with open(manifest_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(manifest_lines))
    print(f"Wrote {manifest_path}!")

    # State
    state_lines = [
        'format_version = 2',
        '',
        '[[package]]',
        'id = "re3.weapons.custom"',
        'version = "1.0.0"',
        '',
        '[[feature]]',
        'package_id = "re3.weapons.custom"',
        'id = "tactical-knife"',
        'enabled = true',
        '',
        '[[feature]]',
        'package_id = "re3.weapons.custom"',
        'id = "tactical-handgun"',
        'enabled = true',
        ''
    ]

    state_path = r'ResidentEvil3-PCPort\mods\state.toml'
    with open(state_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(state_lines))
    print(f"Wrote {state_path}!")

if __name__ == '__main__':
    main()
