#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""用现有主机C编译器执行真实IMU探针的故障注入测试。"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
compiler = os.environ.get('CC') or shutil.which('cc') or shutil.which('gcc')
if not compiler:
    raise SystemExit('需要现有主机C编译器；Windows可在已有WSL中执行本脚本。')
with tempfile.TemporaryDirectory(prefix='imu-probe-test-') as folder:
    exe = Path(folder) / 'probe-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'tests/imu_probe'), '-I' + str(ROOT / 'src'),
                    str(ROOT / 'tests/imu_probe/harness.c'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
