import os
import struct
import zlib

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

def decode_tim_to_png(tim_data, out_prefix):
    tag, bpp = struct.unpack('<II', tim_data[:8])
    pos = 8
    has_clut = (bpp & 8) != 0
    mode = bpp & 7
    palettes = []
    if has_clut:
        clut_len, clut_x, clut_y, clut_w, clut_h = struct.unpack('<I4H', tim_data[pos:pos+12])
        pos += 12
        num_colors = clut_w * clut_h
        raw_colors = tim_data[pos:pos + num_colors * 2]
        pos += num_colors * 2
        for pal_idx in range(clut_h):
            pal = []
            for c_idx in range(clut_w):
                c_off = (pal_idx * clut_w + c_idx) * 2
                c_val = struct.unpack('<H', raw_colors[c_off:c_off+2])[0]
                r = (c_val & 0x1F) << 3
                g = ((c_val >> 5) & 0x1F) << 3
                b = ((c_val >> 10) & 0x1F) << 3
                # semi-transparency check
                stp = (c_val >> 15) & 1
                if c_val == 0:
                    a = 0
                else:
                    a = 255
                pal.append((r, g, b, a))
            palettes.append(pal)
            
    img_len, img_x, img_y, img_w, img_h = struct.unpack('<I4H', tim_data[pos:pos+12])
    pos += 12
    raw_img = tim_data[pos:pos + img_w * img_h * 2]
    
    real_w = img_w * (2 if mode == 1 else 4 if mode == 0 else 1)
    print(f"Decoding TIM: mode={mode}, palettes={len(palettes)}, dim={real_w}x{img_h}")
    
    for p_idx, pal in enumerate(palettes):
        pixels = []
        for y in range(img_h):
            row = []
            row_start = y * (img_w * 2)
            for x in range(real_w):
                color_idx = raw_img[row_start + x]
                if color_idx < len(pal):
                    row.append(pal[color_idx])
                else:
                    row.append((0, 0, 0, 0))
            pixels.append(row)
        out_name = f"{out_prefix}_pal{p_idx}.png"
        save_png(real_w, img_h, pixels, out_name)
        print(f"  Saved {out_name}")

if __name__ == "__main__":
    with open(r'RE3Recomp\tools\model_tool\extracted_stage1\R101.BIN', 'rb') as f:
        data = f.read()
    decode_tim_to_png(data[0x04E800:0x04E800+99872], r'RE3Recomp\tools\model_tool\extracted_stage1\pair1')
    decode_tim_to_png(data[0x08A800:0x08A800+66592], r'RE3Recomp\tools\model_tool\extracted_stage1\pair2')
    decode_tim_to_png(data[0x0AD000:0x0AD000+66592], r'RE3Recomp\tools\model_tool\extracted_stage1\pair3')
