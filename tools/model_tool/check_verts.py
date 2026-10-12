import struct

with open(r'RE3Recomp\tools\model_tool\models\zombie_54\orig_z54.EMD', 'rb') as f:
    orig = f.read()

dir_off = struct.unpack('<I', orig[:4])[0]
offsets = [struct.unpack('<I', orig[dir_off+i*4:dir_off+i*4+4])[0] for i in range(15)]
s14_off = offsets[14]
v0 = s14_off + 8 + 0x1BC0
print("Bytes at 8+0x1BC0 in orig:")
for i in range(5):
    chunk = orig[v0 + i*8 : v0 + (i+1)*8]
    print(f"  v[{i}]: {chunk.hex()} -> {struct.unpack('<4h', chunk)}")

with open(r'RE3Recomp\tools\model_tool\models\zombie_54\zombie_54.obj', 'r') as f:
    lines = [l.strip() for l in f if l.startswith('v ')]
print("First 5 lines in OBJ:")
for l in lines[:5]:
    print(" ", l)
