#!/usr/bin/env python3
"""Render PBM frame buffers as an enlarged OLED-style contact sheet."""
import sys, glob, os
from PIL import Image, ImageDraw, ImageFont

def load(p):
    lines = [l for l in open(p).read().split("\n") if l and not l.startswith("#")]
    w, h = map(int, lines[1].split())
    rows = lines[2:2 + h]
    return w, h, rows

def oled(p, s=5):
    w, h, rows = load(p)
    pad = 10
    img = Image.new("RGB", (w * s + 2 * pad, h * s + 2 * pad), (8, 8, 10))
    d = ImageDraw.Draw(img)
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            x0, y0 = pad + x * s, pad + y * s
            col = (235, 245, 255) if c == "1" else (20, 22, 26)
            d.rectangle([x0, y0, x0 + s - 2, y0 + s - 2], fill=col)
    return img

files = sorted(glob.glob(os.path.join(sys.argv[1], "*.pbm")))
tiles = [(os.path.basename(f)[3:-4].replace("_", " "), oled(f)) for f in files]
tw, th = tiles[0][1].size
cols = 2
rows = (len(tiles) + cols - 1) // cols
lab = 30
sheet = Image.new("RGB", (cols * tw + (cols + 1) * 20, rows * (th + lab) + (rows + 1) * 20), (245, 245, 247))
d = ImageDraw.Draw(sheet)
try:
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 18)
except Exception:
    font = ImageFont.load_default()
for i, (name, im) in enumerate(tiles):
    cx = 20 + (i % cols) * (tw + 20)
    cy = 20 + (i // cols) * (th + lab + 20)
    d.text((cx, cy), name.capitalize(), fill=(40, 40, 45), font=font)
    sheet.paste(im, (cx, cy + lab))
sheet.save(sys.argv[2])
print("saved", sys.argv[2], sheet.size)
