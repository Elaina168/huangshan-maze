#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""将真实LVGL导出的字形与源字体的独立栅格结果逐像素核对。"""
import argparse
import ast
import json
from pathlib import Path
import re
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font', type=Path, required=True)
    parser.add_argument('--dump', type=Path, required=True)
    args = parser.parse_args()
    subprocess.run([sys.executable, str(ROOT/'scripts/generate_ui_font.py'),
                    '--font', str(args.font), '--check'], check=True)
    manifest = json.loads((ROOT/'docs/design/chinese-ui-font.json').read_text(encoding='utf-8'))
    source = (ROOT/'src/maze_ui.c').read_text(encoding='utf-8')
    text = ''.join(ast.literal_eval(s) for s in re.findall(r'"(?:[^"\\]|\\.)*"', source))
    expected = {ord(c) for c in text if ord(c)>127}
    assert expected == set(manifest['codepoints'])
    font = ImageFont.truetype(str(args.font), 16)
    seen = set()
    odd_width = 0
    for line in args.dump.read_text(encoding='utf-8').splitlines():
        cp, w, h, x, y, advance, pixels = line.split()
        cp, w, h, x, y, advance = map(int, (cp,w,h,x,y,advance))
        assert cp not in seen
        seen.add(cp)
        left, top, right, bottom = font.getbbox(chr(cp), anchor='ls')
        assert (w,h,x,y,advance) == (right-left,bottom-top,left,-bottom,round(font.getlength(chr(cp))))
        img = Image.new('L', (w,h))
        ImageDraw.Draw(img).text((-left,-top),chr(cp),font=font,anchor='ls',fill=255)
        raw = bytes.fromhex(pixels)
        # LVGL展开后的A8值，每个像素相对原始灰度只允许4bpp量化误差。
        assert len(raw)==w*h
        assert all(abs(a-b)<=8 for a,b in zip(raw,img.get_flattened_data())), f'字形像素错误 U+{cp:04X}'
        odd_width += w%2
    assert seen == expected, f'字形集合不符: {expected-seen}, {seen-expected}'
    assert odd_width > 0
    print(f'PASS: {len(seen)} actual LVGL decoded glyphs match source pixels, including {odd_width} odd-width glyphs')


if __name__ == '__main__':
    main()
