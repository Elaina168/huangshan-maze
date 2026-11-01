#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""使用实际SDK FlashDB和独立进程NOR镜像验证存档，不连接板子。"""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    args = parser.parse_args()
    vendor = args.sdk / 'external/FlashDB'
    compiler = shutil.which('cc')
    assert compiler, '需要已有主机C编译器'
    with tempfile.TemporaryDirectory(prefix='maze-store-') as folder:
        folder = Path(folder)
        exe = folder / 'store-test'
        includes = [ROOT/'tests/maze/store-host', ROOT/'src', vendor/'inc', vendor/'port/fal/inc']
        sources = [ROOT/'tests/maze/store-host/nor_harness.c', ROOT/'src/maze_store.c',
                   ROOT/'src/maze_archive.c', vendor/'src/fdb.c', vendor/'src/fdb_kvdb.c', vendor/'src/fdb_utils.c']
        command = [compiler, '-std=c99', '-Wall', '-Wextra', '-Wno-unused-parameter',
                   *['-I'+str(x) for x in includes], *map(str, sources), '-o', str(exe)]
        subprocess.run(command, check=True)

        def run(mode, image, *extra, codes=(0,)):
            result = subprocess.run([str(exe), mode, str(image), *map(str, extra)],
                                    capture_output=True, text=True, timeout=15)
            assert result.returncode in codes, (mode, extra, result.returncode, result.stdout, result.stderr)
            return result

        def state(result):
            return bytes.fromhex(re.search(r'STATE ([0-9a-f]+)', result.stdout)[1])

        image = folder/'flash.bin'
        image.write_bytes(b'\xff'*65536)
        initial = state(run('init', image))
        assert int.from_bytes(initial[8:16], 'little') == 0
        first = state(run('save', image, 1))
        assert state(run('boot', image)) == first
        run('retry', image, 2)
        assert int.from_bytes(state(run('boot', image))[8:16], 'little') == 2
        print('PASS: actual FlashDB initialization, reboot, alternating commit and failed-write retry')
        # 未知非空区域不能被格式化成新用户。
        unknown = folder/'unknown.bin'
        raw = bytearray(b'\xff'*65536); raw[0] = 0
        unknown.write_bytes(raw)
        run('boot', unknown, codes=(2,))
        assert unknown.read_bytes() == raw
        print('PASS: unknown nonempty NOR preserved without formatting')
        for mode, expected in [('version', -3002), ('conflict', -3103)]:
            special = folder/(mode+'.bin')
            shutil.copyfile(image,special)
            run(mode,special)
            before=special.read_bytes()
            result=run('boot',special,codes=(2,))
            assert f'ERROR {expected}' in result.stdout
            assert special.read_bytes()==before
        run('readerror',image,codes=(2,))
        run('partialretry',image,3)
        print('PASS: valid future-version and revision-conflict preserved; read/partial-write failures explicit')

        # 一次普通提交和第一次实际GC提交都从可复现的已提交镜像重放。
        fixtures=[]
        for rev in range(4,260):
            before=image.read_bytes()
            old=state(run('boot',image))
            result=run('save',image,rev)
            count=int(re.search(r'OPS (\d+)',result.stdout)[1])
            erased=int(re.search(r'ERASES (\d+)',result.stdout)[1])
            if rev==4 or erased:
                fixtures.append(('gc' if erased else 'ordinary',before,old,state(result),rev,count))
            if erased:
                assert rev>4, '普通提交不应重建已存在数据库'
                break
        else:
            raise AssertionError('未实际观察到初始化之后的GC擦除')
        cuts=0
        for label,before,old,new,rev,count in fixtures:
            probe=folder/'probe.bin';probe.write_bytes(before)
            active=int(re.search(r'ACTIVE (\d+)',run('boot',probe).stdout)[1])
            for operation in range(1,count+1):
                for prefix in range(5):
                    probe.write_bytes(before)
                    run('save',probe,rev,operation,prefix,codes=(77,))
                    start=active*32768
                    assert probe.read_bytes()[start:start+32768]==before[start:start+32768]
                    recovered=state(run('boot',probe))
                    assert recovered in (old,new), (label,operation,prefix)
                    cuts+=1
            print(f'PASS: {label} {count} write/erase operations x 5 cut positions -> complete old/new archive',flush=True)
        # 已写完整但读回失败时，重试仍只提交同一档案。
        probe=folder/'readback.bin';probe.write_bytes(fixtures[0][1])
        normal=run('save',probe,fixtures[0][4])
        read_count=int(re.search(r'READS (\d+)',normal.stdout)[1])
        probe.write_bytes(fixtures[0][1])
        run('readback',probe,fixtures[0][4],read_count)
        assert state(run('boot',probe))==fixtures[0][3]
        print('PASS: final readback failure then same-revision retry preserves one logical snapshot')

        virgin=folder/'virgin.bin';virgin.write_bytes(b'\xff'*65536)
        initialized=run('init',virgin)
        count=int(re.search(r'OPS (\d+)',initialized.stdout)[1])
        for operation in range(1,count+1):
            for prefix in range(5):
                virgin.write_bytes(b'\xff'*65536)
                run('init',virgin,0,operation,prefix,codes=(77,))
                before=virgin.read_bytes()
                result=run('boot',virgin,codes=(0,2))
                if result.returncode==0:
                    assert state(result)==initial
                else:
                    after=virgin.read_bytes()
                    differences=[i for i,(a,b) in enumerate(zip(before,after)) if a!=b]
                    assert not differences, (operation,prefix,result.stdout,differences[:30],
                                             before[differences[0]:differences[0]+16].hex(),
                                             after[differences[0]:differences[0]+16].hex()) if differences else None
                cuts+=1
        print(f'PASS: first initialization {count} operations x 5 cuts -> valid default or explicit preserved error')
        print(f'PASS: {cuts} forced-process cuts; NOR 1-to-0 writes, aligned erases and protected source bank')
        print(json.dumps({'vendor_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                          for p in sources[-3:]}, 'command': command}))


if __name__ == '__main__':
    main()
