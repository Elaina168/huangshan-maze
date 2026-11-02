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
sources = ['maze_navigation.c', 'maze_archive.c', 'maze_progress.c', 'maze_catalog.c',
           'maze_app.c', 'maze_model.c', 'maze_level.c', 'maze_map.c',
           'maze_map_frost_fork.c', 'maze_map_second.c', 'maze_map_ice_loops.c', 'maze_map_mechanism_trial.c']
with tempfile.TemporaryDirectory(prefix='maze-main-') as folder:
    exe = Path(folder) / 'main-runtime'
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror',
                    *['-I'+str(root/p) for p in ['tests/maze/main-host', 'tests/maze', 'tests/maze/store-host', 'src']],
                    '-I'+str(vendor/'inc'), '-I'+str(vendor/'port/fal/inc'),
                    str(root/'tests/maze/main-host/main_runtime_harness.c'),
                    *[str(root/'src'/p) for p in sources], '-lm', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
