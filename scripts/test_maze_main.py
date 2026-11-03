#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""运行正式入口I/O替身测试；实际FlashDB断电模型另由test_maze_store覆盖。"""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
vendor = args.sdk / 'external/FlashDB'
sources = ['maze_navigation.c', 'maze_random.c', 'maze_random_templates.c', 'maze_archive.c', 'maze_progress.c', 'maze_catalog.c',
           'maze_app.c', 'maze_model.c', 'maze_level.c', 'maze_map.c',
           'maze_map_ice_loops.c', 'maze_map_mechanism_trial.c', 'maze_map_dynamic_trial.c']
with tempfile.TemporaryDirectory(prefix='maze-main-') as folder:
    exe = Path(folder) / 'main-runtime'
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Wno-missing-field-initializers', '-Werror',
                    *['-I'+str(root/p) for p in ['tests/maze/main-host', 'tests/maze', 'tests/maze/store-host', 'src']],
                    '-I'+str(vendor/'inc'), '-I'+str(vendor/'port/fal/inc'),
                    str(root/'tests/maze/main-host/main_runtime_harness.c'),
                    *[str(root/'src'/p) for p in sources], '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

# 实际板配置RT_CONSOLEBUF_SIZE=128；诊断最宽整数也必须保留换行。
import ast,re
source=(root/'src/main.c').read_text(encoding='utf-8')
for literal in re.findall(r'rt_kprintf\("((?:[^"\\]|\\.)*)"',source):
    fmt=ast.literal_eval('"'+literal+'"')
    if not (fmt.startswith('[maze_timing]') or fmt.startswith('[maze_motion]') or
            fmt.startswith('[maze_random]') or fmt.startswith('[maze_health]') or fmt.startswith('[maze_mech]') or fmt.startswith('[maze] gen=')):
        continue
    worst=re.sub(r'%l?[udi]',lambda m:'-2147483648' if m[0].endswith('d') or m[0].endswith('i') else '4294967295',fmt)
    assert len(worst.encode('utf-8'))<128,(len(worst.encode('utf-8')),fmt)
print('PASS: timing/state diagnostics retain newline within actual 128-byte console buffer')
