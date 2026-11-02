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
    for name in ['archive_harness', 'archive_migration_harness']:
        archive_exe = Path(folder) / name
        subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze' / (name + '.c')),
                        str(ROOT / 'src/maze_archive.c'), '-o', str(archive_exe)], check=True)
        subprocess.run([str(archive_exe), str(ROOT / 'tests/maze/fixtures/archive-v1.bin'), str(ROOT / 'tests/maze/fixtures/archive-v2.bin')], check=True)
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
                    str(ROOT / 'tests/maze/legacy_levels.c'),
                    str(ROOT / 'src/maze_model.c'), str(ROOT / 'src/maze_level.c'),
                    '-lm', '-o', str(inertia_exe)], check=True)
    subprocess.run([str(inertia_exe)], check=True)
    model_exe = Path(folder) / 'model-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/model_harness.c'),
                    str(ROOT / 'tests/maze/legacy_levels.c'),
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
                    str(ROOT / 'src/maze_map.c'), str(ROOT / 'src/maze_map_frost_fork.c'),
                    str(ROOT / 'src/maze_model.c'), str(ROOT / 'src/maze_level.c'),
                    '-lm', '-o', str(map_exe)], check=True)
    subprocess.run([str(map_exe)], check=True)
    load_exe = Path(folder) / 'map-load-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/map_load_harness.c'),
                    str(ROOT / 'src/maze_map.c'), str(ROOT / 'src/maze_map_frost_fork.c'),
                    '-lm', '-o', str(load_exe)], check=True)
    subprocess.run([str(load_exe)], check=True)
    navigation_exe = Path(folder) / 'navigation-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/navigation_harness.c'),
                    str(ROOT / 'src/maze_navigation.c'), str(ROOT / 'src/maze_app.c'),
                    str(ROOT / 'src/maze_archive.c'), str(ROOT / 'src/maze_progress.c'),
                    str(ROOT / 'src/maze_map.c'), str(ROOT / 'src/maze_model.c'),
                    str(ROOT / 'src/maze_level.c'), '-lm', '-o', str(navigation_exe)], check=True)
    subprocess.run([str(navigation_exe)], check=True)
    second_exe = Path(folder) / 'second-map-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/second_map_harness.c'),
                    str(ROOT / 'src/maze_map_second.c'), str(ROOT / 'src/maze_map.c'),
                    str(ROOT / 'src/maze_model.c'), str(ROOT / 'src/maze_level.c'),
                    '-lm', '-o', str(second_exe)], check=True)
    subprocess.run([str(second_exe)], check=True)
    progress_exe = Path(folder) / 'progress-test'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT / 'src'), str(ROOT / 'tests/maze/progress_harness.c'),
                    str(ROOT / 'src/maze_progress.c'), str(ROOT / 'src/maze_archive.c'),
                    '-o', str(progress_exe)], check=True)
    subprocess.run([str(progress_exe)], check=True)

    for name, sources, flags in [
        ('camera', ['camera_harness.c'], []),
        ('scroll-trial', ['scroll_trial_harness.c'], ['-DMAZE_SCROLL_TRIAL']),
    ]:
        source_files = [ROOT / 'tests/maze' / file for file in sources]
        source_files += [ROOT / 'src' / file for file in ['maze_model.c', 'maze_level.c']]
        source_files += [ROOT / 'src' / ('maze_camera.c' if name == 'camera' else 'maze_map_scroll_trial.c')]
        if name != 'camera':
            source_files.append(ROOT / 'src/maze_map.c')
        executable = Path(folder) / name
        subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror',
                        '-I' + str(ROOT / 'src'), *flags, *map(str, source_files), '-lm', '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)

    runner = Path(folder) / 'scroll-runner'
    subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror', '-DMAZE_SCROLL_TRIAL',
                    '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'tests/maze'),
                    str(ROOT / 'tests/maze/scroll_runner_harness.c'),
                    *[str(ROOT / 'src' / file) for file in ['maze_scroll_trial.c', 'maze_app.c', 'maze_model.c',
                      'maze_level.c', 'maze_map.c', 'maze_map_scroll_trial.c']], '-lm', '-o', str(runner)], check=True)
    for mode in [[], ["unchunked"], ["starved"], ["recovery"], ["bounded-single"]]:
        subprocess.run([str(runner), *mode], check=True)

    for name, harness, extras in [
        ('third-map', 'third_map_harness.c', []),
        ('stage4-core', 'stage4_harness.c', ['maze_app.c']),
        ('third-navigation', 'third_navigation_harness.c', ['maze_navigation.c', 'maze_progress.c', 'maze_archive.c', 'maze_app.c',
                                                       'maze_catalog.c', 'maze_map_frost_fork.c', 'maze_map_second.c', 'maze_map_mechanism_trial.c']),
    ]:
        executable = Path(folder) / name
        sources = ['maze_map_ice_loops.c', 'maze_map.c', 'maze_model.c', 'maze_level.c', *extras]
        if name == 'stage4-core':
            sources.append('maze_map_mechanism_trial.c')
        subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror', '-I' + str(ROOT / 'src'),
                        str(ROOT / 'tests/maze' / harness), *[str(ROOT / 'src' / file) for file in sources],
                        '-lm', '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)

    for name, sources in [
        ('stage4-progress', ['maze_progress.c', 'maze_archive.c']),
    ]:
        executable = Path(folder) / name
        subprocess.run([compiler, '-std=c99', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'src'),
                        str(ROOT/'tests/maze/stage4_progress_harness.c'),
                        *[str(ROOT/'src'/file) for file in sources], '-lm', '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
