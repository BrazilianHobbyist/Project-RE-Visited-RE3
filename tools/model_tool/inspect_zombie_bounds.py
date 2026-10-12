def inspect_obj_bounds(obj_path):
    print(f"=== Bounds for {obj_path} ===")
    current_group = "default"
    groups = {}
    with open(obj_path, 'r') as f:
        for line in f:
            if line.startswith('g '):
                current_group = line.strip().split()[1]
                if current_group not in groups:
                    groups[current_group] = []
            elif line.startswith('v '):
                parts = [float(p) for p in line.strip().split()[1:4]]
                if current_group not in groups:
                    groups[current_group] = []
                groups[current_group].append(parts)
                
    for gname, vlist in groups.items():
        if not vlist: continue
        xs = [v[0] for v in vlist]
        ys = [v[1] for v in vlist]
        zs = [v[2] for v in vlist]
        print(f"  {gname:18} | Count: {len(vlist):3d} | X: [{min(xs):6.2f}, {max(xs):6.2f}] | Y: [{min(ys):6.2f}, {max(ys):6.2f}] | Z: [{min(zs):6.2f}, {max(zs):6.2f}]")

if __name__ == "__main__":
    inspect_obj_bounds(r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54.obj')
    inspect_obj_bounds(r'RE3Recomp\tools\model_tool\models\zombie_59\zombie_59.obj')
