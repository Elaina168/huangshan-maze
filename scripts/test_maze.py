#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""编译真实后台采样源码，以受控外设与调度执行生命周期测试。"""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
assert (ROOT / 'src/maze_imu.c').is_file(), 'FAIL: 后台采样实现尚不存在'
compiler = os.environ.get('CC') or shutil.which('cc') or shutil.which('gcc')
if not compiler:
    raise SystemExit('需要现有主机C编译器；可在已有WSL中执行。')
with tempfile.TemporaryDirectory(prefix='maze-test-') as folder:
    exe = Path(folder) / 'imu-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'tests/maze'), '-I' + str(ROOT / 'tests/imu_probe'),
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/imu_harness.c'),
                    '-o', str(exe)], check=True)
    for scenario in sys.argv[1:] or ['normal', 'late_read', 'stop_during_read', 'stop_during_status', 'read_failed',
                                    'configure_failed', 'restore_failed', 'save_failed', 'bus_missing', 'not_ready', 'queued_failure', 'cadence', 'gap_latch']:
        subprocess.run([str(exe), scenario], check=True)
    app_exe = Path(folder) / 'app-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/harness.c'),
                    str(ROOT / 'src/maze_app.c'), str(ROOT / 'src/maze_model.c'),
                    str(ROOT / 'src/maze_level.c'), '-lm', '-o', str(app_exe)], check=True)
    subprocess.run([str(app_exe)], check=True)
    inertia_exe = Path(folder) / 'inertia-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/inertia_harness.c'),
                    str(ROOT / 'src/maze_model.c'), str(ROOT / 'src/maze_level.c'),
                    '-lm', '-o', str(inertia_exe)], check=True)
    subprocess.run([str(inertia_exe)], check=True)
    model_exe = Path(folder) / 'model-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/model_harness.c'),
                    str(ROOT / 'src/maze_model.c'), str(ROOT / 'src/maze_level.c'),
                    '-lm', '-o', str(model_exe)], check=True)
    subprocess.run([str(model_exe)], check=True)
    state_exe = Path(folder) / 'state-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/state_harness.c'),
                    str(ROOT / 'src/maze_app.c'), str(ROOT / 'src/maze_model.c'),
                    str(ROOT / 'src/maze_level.c'), '-lm', '-o', str(state_exe)], check=True)
    subprocess.run([str(state_exe)], check=True)
    map_exe = Path(folder) / 'map-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/map_harness.c'),
                    str(ROOT / 'src/maze_model.c'), str(ROOT / 'src/maze_level.c'),
                    '-lm', '-o', str(map_exe)], check=True)
    subprocess.run([str(map_exe)], check=True)
