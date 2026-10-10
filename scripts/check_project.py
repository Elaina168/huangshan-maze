#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""核对课程文档追踪、导入证据、提交规范和开发日志，不代替真机验收。"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
TYPES = {'feat', 'fix', 'docs', 'test', 'refactor', 'perf', 'build', 'ci', 'chore', 'revert', 'hw'}


def check_message(message):
    """只校验真实提交的格式，不推定正文内容已验证。"""
    lines = message.strip().splitlines()
    errors = []
    match = re.fullmatch(r'([a-z]+)\(([^\s()]+)\): (\S.*)', lines[0] if lines else '')
    if not match or match.group(1) not in TYPES:
        errors.append('提交标题必须为允许类型(模块): 具体变化')
    if len(lines) < 2 or lines[1].strip():
        errors.append('标题与正文之间必须空一行')
    sections = {}
    for line in lines[2:]:
        label = re.match(r'^(背景|改动|验证|AI协作):\s*(.*)', line)
        if label:
            sections[label.group(1)] = label.group(2).strip()
    for label in ['背景', '改动', '验证', 'AI协作']:
        if not sections.get(label):
            errors.append(f'缺少非空正文项：{label}')
    if sections.get('AI协作') and not sections['AI协作'].startswith(('是', '否')):
        errors.append('AI协作必须明确是或否')
    return errors


def check_project():
    errors = []
    required = ['AGENTS.md', 'README.md', 'THIRD_PARTY.md', 'docs/course-requirements.md',
                'docs/product-requirements.md', 'docs/acceptance-test-plan.md',
                'docs/project-plan.md', 'docs/risks.md', 'project/SConstruct',
                'project/SConscript', 'project/proj.conf', 'src/main.c', 'src/SConscript']
    for name in required:
        if not (ROOT / name).is_file():
            errors.append(f'缺少文件：{name}')
    if errors:
        return errors
    requirements = (ROOT / 'docs/product-requirements.md').read_text(encoding='utf-8')
    tests = (ROOT / 'docs/acceptance-test-plan.md').read_text(encoding='utf-8')
    req_ids = re.findall(r'^\| (REQ-\d{3}) \|', requirements, re.M)
    test_ids = re.findall(r'^\| (TC-\d{3}) \|', tests, re.M)
    if not req_ids or len(req_ids) != len(set(req_ids)):
        errors.append('需求定义为空或编号重复')
    if not test_ids or len(test_ids) != len(set(test_ids)):
        errors.append('测试定义为空或编号重复')
    if set(re.findall(r'REQ-\d{3}', tests)) != set(req_ids):
        errors.append('测试需求引用未覆盖全部需求，或引用了未定义需求')
    if set(re.findall(r'TC-\d{3}', requirements)) - set(test_ids):
        errors.append('需求引用了未定义测试')
    for row in re.findall(r'^\| REQ-\d{3} .*$', requirements, re.M):
        if not re.search(r'TC-\d{3}', row):
            errors.append('存在未关联用例的需求')
    markdown_files = [ROOT / 'README.md', ROOT / 'AGENTS.md', ROOT / 'THIRD_PARTY.md'] + list((ROOT / 'docs').rglob('*.md')) + list((ROOT / 'evidence').rglob('*.md'))
    for path in markdown_files:
        content = path.read_text(encoding='utf-8', errors='strict')
        for target in re.findall(r'\]\(([^)]+)\)', content):
            if re.match(r'^[a-z]+://', target) or target.startswith('#'):
                continue
            target = target.split('#', 1)[0]
            if re.match(r'^[A-Za-z]:', target):
                errors.append(f'不可移植的本机文档链接：{path.relative_to(ROOT)}')
            elif not (path.parent / target).exists():
                errors.append(f'无效链接：{path.relative_to(ROOT)} -> {target}')
    manifest = json.loads((ROOT / 'docs/references/import-manifest.json').read_text(encoding='utf-8'))
    for item in manifest:
        path = ROOT / item['path']
        if not path.is_file():
            errors.append(f'导入证据缺失：{item["path"]}')
        elif not item['modified'] and hashlib.sha256(path.read_bytes()).hexdigest() != item['sha256']:
            errors.append(f'原样导入文件校验不符：{item["path"]}')
    logs = list((ROOT / 'docs/devlog').glob('????-??-??.md'))
    if not logs:
        errors.append('缺少开发日志')
    for log in logs:
        content = log.read_text(encoding='utf-8')
        for section in ['今日完成', '问题与决策', '验证', 'AI协作', '后续计划']:
            if f'## {section}' not in content:
                errors.append(f'{log.name} 缺少日志节：{section}')
    return errors


def check_history():
    errors = []
    result = subprocess.run(['git', 'log', '--format=%H%x1f%ad%x1f%s%x1f%b%x1e', '--date=format:%Y-%m-%d'], cwd=ROOT, capture_output=True, encoding='utf-8')
    if result.returncode:
        return ['无法读取提交历史']
    for record in result.stdout.split('\x1e'):
        if not record.strip():
            continue
        sha, date, subject, body = record.strip('\n').split('\x1f', 3)
        for error in check_message(subject + '\n\n' + body):
            errors.append(f'{sha[:7]}：{error}')
        if not (ROOT / 'docs/devlog' / f'{date}.md').is_file():
            errors.append(f'{sha[:7]} 提交日缺少开发日志')
    return errors


def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, 'reconfigure'):
            stream.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser()
    parser.add_argument('--commit-message', type=Path)
    parser.add_argument('--history', action='store_true')
    args = parser.parse_args()
    errors = check_project()
    if args.commit_message:
        errors += check_message(args.commit_message.read_text(encoding='utf-8'))
    if args.history:
        errors += check_history()
    if errors:
        for error in errors:
            print('FAIL:', error)
        return 1
    print('PASS：需求追踪、文档链接、原始证据、开发日志及所选提交检查通过。')
    print('范围：这是工程材料检查，不是产品或硬件验收。')
    return 0


if __name__ == '__main__':
    sys.exit(main())
