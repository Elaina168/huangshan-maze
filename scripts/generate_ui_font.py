#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""从实际界面字串生成可复现的16px/4bpp字形；TTF只用于离线构建。"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re

import PIL
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SOURCE_SHA256 = 'ae82f4e2a55e1316a55bcc1d05e9555ce08d8bda07e893b486896b626fd852ff'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font', type=Path, required=True)
    parser.add_argument('--check', action='store_true', help='仅比较当前生成物，不写入')
    args = parser.parse_args()
    assert hashlib.sha256(args.font.read_bytes()).hexdigest() == SOURCE_SHA256, '源字体SHA不符'
    source = (ROOT / 'src/maze_ui.c').read_text(encoding='utf-8')
    strings = [ast.literal_eval(s) for s in re.findall(r'"(?:[^"\\]|\\.)*"', source)]
    chars = sorted({c for s in strings for c in s if ord(c) > 127}, key=ord)
    assert chars, '界面没有中文，拒绝生成空字体'
    font = ImageFont.truetype(str(args.font), 16)
    missing = font.getmask(chr(0x10ffff))
    bitmap = bytearray()
    descriptors = ['    {0}, /* 保留缺字编号 */']
    for c in chars:
        mask = font.getmask(c)
        assert (mask.size, bytes(mask)) != (missing.size, bytes(missing)), f'源字体缺字: {c}'
        left, top, right, bottom = font.getbbox(c, anchor='ls')
        width, height = right-left, bottom-top
        # 使用字形实际边界，按像素连续打包；奇数宽的行也不插入填充。
        assert width > 0 and height > 0
        assert 0 <= 16+top and 16+bottom <= 20, f'字形超出20px行高: {c}'
        img = Image.new('L', (width, height))
        ImageDraw.Draw(img).text((-left, -top), c, font=font, anchor='ls', fill=255)
        values = [(v*15+127)//255 for v in img.get_flattened_data()]
        index = len(bitmap)
        for i in range(0, len(values), 2):
            bitmap.append((values[i] << 4) | (values[i+1] if i+1 < len(values) else 0))
        descriptors.append(f'    {{{index}, {round(font.getlength(c)*16)}, {width}, {height}, {left}, {-bottom}}}, /* U+{ord(c):04X} {c} */')
    first = ord(chars[0])
    code = '''/* 由scripts/generate_ui_font.py生成，请勿手改。
 * 原字体Copyright 2014-2021 Adobe，SIL OFL 1.1。
 * 项目衍生字形名maze_font_zh_16，授权见licenses/NotoSansSC-OFL.txt。
 */
#include "maze_font_zh_16.h"

static const uint8_t bitmap[]={
'''
    code += '\n'.join('    '+','.join(f'0x{v:02x}' for v in bitmap[i:i+16])+',' for i in range(0, len(bitmap), 16))
    code += '\n};\nstatic const lv_font_fmt_txt_glyph_dsc_t glyphs[]={\n'+'\n'.join(descriptors)+'\n};\n'
    code += 'static const uint16_t unicode_list[]={'+','.join(str(ord(c)-first) for c in chars)+'};\n'
    code += f'''static const lv_font_fmt_txt_cmap_t cmap[]={{
    {{.range_start={first},.range_length={ord(chars[-1])-first+1},.glyph_id_start=1,
     .unicode_list=unicode_list,.list_length={len(chars)},.type=LV_FONT_FMT_TXT_CMAP_SPARSE_TINY}}
}};
static const lv_font_fmt_txt_dsc_t font_dsc={{
    .glyph_bitmap=bitmap,.glyph_dsc=glyphs,.cmaps=cmap,.cmap_num=1,.bpp=4,
    .bitmap_format=LV_FONT_FMT_TXT_PLAIN
}};
const lv_font_t maze_font_zh_16={{
    .get_glyph_dsc=lv_font_get_glyph_dsc_fmt_txt,.get_glyph_bitmap=lv_font_get_bitmap_fmt_txt,
    .line_height=20,.base_line=4,.underline_position=-1,.underline_thickness=1,
    .dsc=&font_dsc,.fallback=&lv_font_montserrat_16
}};
'''
    manifest = {
        'font_name': 'maze_font_zh_16', 'source': 'SiFli SDK v2.5.1 external/lvgl_v9/tests/src/test_files/fonts/noto/NotoSansSC-Regular.ttf',
        'source_sha256': SOURCE_SHA256, 'pillow': PIL.__version__, 'pixel_size': 16, 'bpp': 4,
        'line_height': 20, 'base_line': 4, 'glyph_count': len(chars), 'bitmap_bytes': len(bitmap),
        'codepoints': [ord(c) for c in chars], 'characters': ''.join(chars),
    }
    for rel, content in [('src/maze_font_zh_16.c', code),
                         ('docs/design/chinese-ui-font.json', json.dumps(manifest, ensure_ascii=False, indent=2)+'\n')]:
        path = ROOT / rel
        if args.check:
            assert path.read_text(encoding='utf-8') == content, f'需重建字形: {rel}'
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open('w', encoding='utf-8', newline='\n') as out:
                out.write(content)
    print(f'PASS: {len(chars)} glyphs, {len(bitmap)} bitmap bytes; reproducibility check={args.check}')


if __name__ == '__main__':
    main()
