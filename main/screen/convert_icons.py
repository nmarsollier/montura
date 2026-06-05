#!/usr/bin/env python3
"""Convert reference icons to C bitmap arrays — no padding, content only.

Reads from ~/Downloads/icons/, resizes to 40px wide, thresholds B&W,
trims ALL empty rows (leading & trailing), outputs compact C arrays.
"""

from PIL import Image
import os, re

ICONS_DIR = os.path.expanduser("~/Downloads/icons")
TARGET_W = 40

NAME_MAP = {
    "no_wifi":  "BIG_WIFI_OFF",
    "manual":   "BIG_ALERT",
    "stop":     "BIG_STOP",
    "parked":   "BIG_PARKED",
    "sideral":  "BIG_STAR",
    "linar":    "BIG_MOON",
    "solar":    "BIG_SUN",
    "ready":    "BIG_READY",
    "slewing":  "BIG_SLEWING",
}


def convert_image(path, name):
    img = Image.open(path).convert("L")

    w, h = img.size
    new_h = int(h * TARGET_W / w)
    img = img.resize((TARGET_W, new_h), Image.LANCZOS)

    pixels = list(img.getdata())

    avg = sum(pixels) / len(pixels)
    invert = avg > 128
    threshold = 128

    bpr = (TARGET_W + 7) // 8
    empty_row = [0] * bpr

    all_rows = []
    for row in range(new_h):
        vals = []
        for b in range(bpr):
            val = 0
            for bit in range(8):
                col = b * 8 + bit
                if col < TARGET_W:
                    pixel = pixels[row * TARGET_W + col]
                    on = pixel < threshold if invert else pixel >= threshold
                    if on:
                        val |= (1 << (7 - bit))
            vals.append(val)
        all_rows.append(vals)

    # Trim leading empty rows
    while all_rows and all_rows[0] == empty_row:
        all_rows.pop(0)

    # Trim trailing empty rows
    while all_rows and all_rows[-1] == empty_row:
        all_rows.pop()

    final_h = len(all_rows)

    if final_h == 0:
        return f"/* WARNING: {name} is empty after trimming */\n"

    hex_rows = []
    for r in all_rows:
        hex_rows.append("    " + ", ".join(f"0x{b:02X}" for b in r) + ",")

    body = "\n".join(hex_rows)

    return f"""static const uint8_t {name}_DATA[] = {{
{body}
}};

const ScreenBitmap {name} = {{
    .width = {TARGET_W},
    .height = {final_h},
    .data = {name}_DATA,
}};
"""


if __name__ == "__main__":
    print("/* Auto-generated from ~/Downloads/icons/ — no padding */")
    print("/* Format: row-major, MSB-left byte packing */")
    print()

    for filename, c_name in NAME_MAP.items():
        found = None
        for ext in ('.png', '.jpeg', '.jpg'):
            path = os.path.join(ICONS_DIR, filename + ext)
            if os.path.exists(path):
                found = path
                break
        if not found:
            print(f"/* WARNING: {filename} not found */")
            continue

        code = convert_image(found, c_name)
        print(f"/* ─── {c_name} (from {os.path.basename(found)}) ─── */")
        print(code)
        print()
