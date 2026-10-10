#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""从原样源码快照编译，避免中文路径写入 Kconfig；不自动烧录。"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
BOARD = 'sf32lb52-lchspi-ulp'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def collect_sources():
    """按相同规则枚举完整源码，包含新增文件并排除生成文件。"""
    sources = []
    for folder in ['project', 'src']:
        for path in sorted((ROOT / folder).rglob('*')):
            rel = path.relative_to(ROOT)
            if not path.is_file() or any(p.startswith('build_') or p in {'__pycache__', 'sifli_sdk'} for p in rel.parts):
                continue
            if folder == 'project' and (rel.parts[1] == 'src' or path.name == '.sconsign.dblite'):
                continue
            sources.append({'path': rel.as_posix(), 'sha256': digest(path)})
    return sources


def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser()
    parser.add_argument('-j', type=int, default=8)
    args = parser.parse_args()
    if args.j < 1:
        parser.error('并行数必须大于零')
    sdk = os.environ.get('SIFLI_SDK')
    scons = shutil.which('scons')
    if not sdk or not scons:
        print('SDK 未激活：先运行实际 SDK 的 export.ps1。')
        return 2
    # 新建独立快照，不修改已有工程或 SDK，不执行递归删除。
    stage = Path(tempfile.mkdtemp(prefix='huangshan-maze-'))
    if not str(stage).isascii() or ' ' in str(stage):
        print('临时目录不是纯英文无空格路径；请将 TEMP/TMP 设置为自己可写的英文目录后重试。')
        return 2
    sources = collect_sources()
    for item in sources:
        destination = stage / item['path']
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / item['path'], destination)
        assert digest(destination) == item['sha256'], item['path']
    out = ROOT / 'build'
    out.mkdir(exist_ok=True)
    state = {'board': BOARD, 'sdk_path': sdk, 'stage_path': str(stage), 'sources': sources,
             'returncode': None, 'artifacts': [], 'hardware_verified': False}
    command = [scons, f'--board={BOARD}', f'-j{args.j}']
    print('ASCII 快照工程：', stage)
    print('执行：', ' '.join(command))
    with (out / 'last_build.log').open('w', encoding='utf-8', newline='\n') as log:
        process = subprocess.Popen(command, cwd=stage / 'project', stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, text=True, encoding='utf-8', errors='replace')
        for line in process.stdout:
            print(line, end='')
            log.write(line)
        state['returncode'] = process.wait()
    build = stage / 'project' / f'build_{BOARD}_hcpu'
    for rel in ['main.bin', 'bootloader/bootloader.bin', 'ftab/ftab.bin']:
        path = build / rel
        if path.is_file():
            state['artifacts'].append({'path': str(path), 'sha256': digest(path), 'bytes': path.stat().st_size})
    # 构建期间原始源码若变化，该产物不能用来证明当前源码通过。
    try:
        unchanged = collect_sources() == sources
    except OSError:
        # 枚举或读取期间源码消失也使证据失效，并正常记录失败状态。
        unchanged = False
    state['source_unchanged'] = unchanged
    (out / 'build-state.json').write_text(json.dumps(state, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    if state['returncode'] != 0 or len(state['artifacts']) != 3 or not unchanged:
        print('FAIL：检查 build/last_build.log 与 build/build-state.json；未烧录。')
        return 1
    print('PASS：构建退出码 0、三项固件存在且原始源码未变；未烧录。')
    return 0


if __name__ == '__main__':
    sys.exit(main())
