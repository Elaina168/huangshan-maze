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
                                    'configure_failed', 'restore_failed', 'save_failed', 'bus_missing', 'not_ready', 'queued_failure']:
        subprocess.run([str(exe), scenario], check=True)
    app_exe = Path(folder) / 'app-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/harness.c'),
                    str(ROOT / 'src/maze_app.c'), '-o', str(app_exe)], check=True)
    subprocess.run([str(app_exe)], check=True)
