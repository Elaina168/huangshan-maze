#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""验证构建期间增删改源码会使快照失效，不启动真实工具链。"""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location('build_firmware', Path(__file__).with_name('build_firmware.py'))
BUILD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BUILD)


class SnapshotTests(unittest.TestCase):
    def run_build(self, mutation, variant=None):
        with tempfile.TemporaryDirectory(prefix='snapshot-test-') as scratch:
            root = Path(scratch) / 'repo'
            stage = Path(scratch) / 'stage'
            (root / 'src').mkdir(parents=True)
            (root / 'project').mkdir()
            stage.mkdir()
            (root / 'src/main.c').write_text('int main(void) { return 0; }\n', encoding='utf-8')
            (root / 'project/SConstruct').write_text('# test\n', encoding='utf-8')

            def process(*args, **kwargs):
                self.child_environment = kwargs.get('env', {})
                def finish():
                    build = Path(kwargs['cwd']) / f'build_{BUILD.BOARD}_hcpu'
                    for rel in ['main.bin', 'bootloader/bootloader.bin', 'ftab/ftab.bin']:
                        target = build / rel
                        target.parent.mkdir(parents=True, exist_ok=True)
                        target.write_bytes(b'fixture firmware')
                    mutation(root)
                    return 0
                return SimpleNamespace(stdout=['fixture build\n'], wait=finish)

            with patch.object(BUILD, 'ROOT', root), patch.object(BUILD.tempfile, 'mkdtemp', return_value=str(stage)), patch.object(BUILD.shutil, 'which', return_value='fixture-scons'), patch.object(BUILD.subprocess, 'Popen', side_effect=process), patch.dict(BUILD.os.environ, {'SIFLI_SDK': 'fixture-sdk'}), patch('sys.argv', (['build_firmware.py'] + (['--variant', variant] if variant else []))), contextlib.redirect_stdout(io.StringIO()):
                result = BUILD.main()
            state = json.loads((root / 'build/build-state.json').read_text(encoding='utf-8'))
            self.build_state = state
            return result, state['source_unchanged']

    def test_default_clears_inherited_trial(self):
        with patch.dict(BUILD.os.environ, {'MAZE_BUILD_VARIANT': 'scrolling-trial'}):
            self.assertEqual(self.run_build(lambda root: None), (0, True))
        self.assertEqual(self.child_environment['MAZE_BUILD_VARIANT'], 'normal')
        self.assertEqual(self.build_state['variant'], 'normal')
        self.assertEqual(self.build_state['defines'], [])

    def test_explicit_trial(self):
        self.assertEqual(self.run_build(lambda root: None, 'scrolling-trial'), (0, True))
        self.assertEqual(self.child_environment['MAZE_BUILD_VARIANT'], 'scrolling-trial')
        self.assertEqual(self.build_state['variant'], 'scrolling-trial')
        self.assertEqual(self.build_state['defines'], ['MAZE_SCROLL_TRIAL'])

    def test_unchanged_tree_passes(self):
        self.assertEqual(self.run_build(lambda root: None), (0, True))

    def test_added_source_invalidates_build(self):
        self.assertEqual(self.run_build(lambda root: (root / 'src/new.c').write_text('/* 新源码 */\n', encoding='utf-8')), (1, False))

    def test_deleted_source_invalidates_build(self):
        self.assertEqual(self.run_build(lambda root: (root / 'src/main.c').unlink()), (1, False))

    def test_modified_source_invalidates_build(self):
        self.assertEqual(self.run_build(lambda root: (root / 'src/main.c').write_text('/* 已修改 */\n', encoding='utf-8')), (1, False))


if __name__ == '__main__':
    unittest.main()
