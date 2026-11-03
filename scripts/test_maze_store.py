#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""使用实际SDK FlashDB和独立进程NOR镜像验证存档，不连接板子。"""
import argparse
import hashlib
import json
import re
import shutil
import struct
import zlib
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
        command = [compiler, '-std=c99', '-Wall', '-Wextra', '-Wno-missing-field-initializers', '-Wno-unused-parameter',
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

        # 真实旧格式经FlashDB写入；RAW检查物理副本，不能被RAM规范化掩盖。
        golden = (ROOT/'tests/maze/fixtures/archive-v1.bin').read_bytes()

        def checksum(data):
            data = bytearray(data)
            data[-4:] = struct.pack('<I', zlib.crc32(data[:-4]))
            return bytes(data)

        def legacy(revision=17, completed=2):
            data = bytearray(golden)
            struct.pack_into('<Q', data, 8, revision)
            data[218 if len(data) == 353 else 382 if len(data) == 517 else 546 if len(data) == 681 else 710] = completed
            return checksum(data)

        def expanded(old, revision=None):
            revision = int.from_bytes(old[8:16], 'little') if revision is None else revision
            if old[4] == 6:
                data=bytearray(old);struct.pack_into('<Q',data,8,revision)
                return checksum(data)
            header = struct.pack('<4sHHQIB', b'HMZ1', 6, 524, revision, 1, 3)
            records={old[21+i*164:21+i*164+32].split(b'\0')[0]: bytearray(old[21+i*164:21+(i+1)*164]) for i in range(old[20])}
            result=[]
            for i,key in enumerate([b'ice_loops',b'mechanism_trial',b'dynamic_trial']):
                if key in records: rec=records[key]
                else: rec=bytearray(struct.pack('<32sBB',key, bool(i and result[i-1][33]),0)+bytes(130))
                if i==0: rec[32]=1
                result.append(rec)
            identity=old[841:848] if old[4]==5 else bytes(7)
            return checksum(header+b''.join(result)+identity+bytes(4))

        def put_blob(image, bank, data):
            file = folder/'blob.dat'
            file.write_bytes(data)
            run('putblob', image, bank, file)

        def raw_state(result):
            return bytes.fromhex(re.search(r'RAW ([0-9a-f]+)', result.stdout)[1])

        def committed(result):
            return int(re.search(r'COMMITTED (\d+)', result.stdout)[1])

        def active(result):
            return int(re.search(r'ACTIVE (\d+)', result.stdout)[1])

        all_migration_cuts = 0
        for old_version in [1, 2, 3, 4, 5]:
            golden = (ROOT/f'tests/maze/fixtures/archive-v{old_version}.bin').read_bytes()
            migration = folder/'migration.bin'
            migration.write_bytes(b'\xff'*65536)
            old = legacy()
            put_blob(migration, 0, old)
            before = migration.read_bytes()
            opened = run('boot', migration)
            assert raw_state(opened) == old and committed(opened) == 17 and 'UPGRADE 1' in opened.stdout
            assert state(opened) == expanded(old, 18) and migration.read_bytes() == before
            migrated = run('migrate', migration, 0)
            assert raw_state(migrated) == expanded(old, 18) and committed(migrated) == 18
            assert 'UPGRADE 0' in migrated.stdout and migration.read_bytes()[:32768] == before[:32768]
            assert state(run('boot', migration)) == expanded(old, 18)
            migration_fixture = ('migration', before, old, raw_state(migrated), 0,
                                 int(re.search(r'OPS (\d+)', migrated.stdout)[1]))
            read_count = int(re.search(r'READS (\d+)', migrated.stdout)[1])
            for mode in ['migrateretry', 'migratereadback']:
                migration.write_bytes(before)
                retried = run(mode, migration, 0, read_count) if mode == 'migratereadback' else run(mode, migration, 0)
                assert raw_state(retried) == expanded(old, 18) and committed(retried) == 18
                assert migration.read_bytes()[:32768] == before[:32768]
            print(f'PASS: v{old_version} open read-only, RAM revision +1 once, v6 migration/readback/write retry idempotent', flush=True)

            cases = [
                ('equal', old, expanded(old), 1, 17, False),
                ('equal-reversed', expanded(old), old, 0, 17, False),
                ('legacy-higher', expanded(legacy(16)), old, 1, 17, True),
                ('current-higher', old, expanded(old, 19), 1, 19, False),
            ]
            permuted = bytearray(expanded(old))
            original = bytes(permuted)
            for i, index in enumerate([2, 0, 1]):
                permuted[21+i*164:21+(i+1)*164] = original[21+index*164:21+(index+1)*164]
            cases.append(('canonical-equal', old, checksum(permuted), 1, 17, False))
            for name, a, b, expected_bank, revision, upgrade in cases:
                test = folder/(name+'.bin')
                test.write_bytes(b'\xff'*65536)
                put_blob(test, 0, a);put_blob(test, 1, b)
                preserved = test.read_bytes()
                result = run('boot', test)
                assert active(result) == expected_bank and committed(result) == revision
                assert ('UPGRADE 1' if upgrade else 'UPGRADE 0') in result.stdout
                assert raw_state(result) == (a if expected_bank == 0 else b) and test.read_bytes() == preserved

            conflicting = bytearray(expanded(old))
            struct.pack_into('<I', conflicting, 383, 99)
            future = bytearray(expanded(old) + bytes(83))
            struct.pack_into('<H', future, 4, 7);struct.pack_into('<H', future, 6, len(future))
            future_rules = bytearray(future);struct.pack_into('<H', future_rules, 4, 3)
            struct.pack_into('<I', future_rules, 16, 2)
            fourth_conflict = bytearray(expanded(old))
            struct.pack_into('<I', fourth_conflict, 219, 88)
            for name, other, expected in [('third-conflict', checksum(conflicting), -3103),
                                         ('fourth-conflict', checksum(fourth_conflict), -3103),
                                         ('future-long', checksum(future), -3002),
                                         ('rules-long', checksum(future_rules), -3002)]:
                test = folder/(name+'.bin');test.write_bytes(b'\xff'*65536)
                put_blob(test, 0, old);put_blob(test, 1, other)
                preserved = test.read_bytes();result = run('boot', test, codes=(2,))
                assert f'ERROR {expected}' in result.stdout and test.read_bytes() == preserved
            maximum = folder/'maximum.bin';maximum.write_bytes(b'\xff'*65536)
            put_blob(maximum, 0, legacy((1 << 64)-1))
            preserved = maximum.read_bytes()
            assert 'ERROR -3104' in run('boot', maximum, codes=(2,)).stdout and maximum.read_bytes() == preserved
            print('PASS: mixed revisions/formats, equal canonical v6 preference, third/fourth conflict, future/rules long blob and UINT64_MAX preserved', flush=True)
            # 实际最大修订v3及迁移恰达最大值均保留RAW；后续业务拒绝由main/progress测试。
            vmax = (1 << 64)-1
            for source in [expanded(legacy(vmax)), legacy(vmax-1)]:
                edge = folder/'maximum-v3.bin';edge.write_bytes(b'\xff'*65536)
                put_blob(edge, 0, source);preserved = edge.read_bytes()
                result = run('boot', edge)
                assert state(result) == (expanded(source, vmax) if source[4] < 6 else source) and edge.read_bytes() == preserved
                if source[4] < 6:
                    result = run('migrate', edge, 0)
                    assert raw_state(result) == expanded(source, vmax)
                    assert edge.read_bytes()[:32768] == preserved[:32768]
                else:
                    assert raw_state(result) == source
            print('PASS: actual SDK direct v3 maximum and legacy maximum-1 migration preserve RAW/source without wrap', flush=True)


            # 旧格式历史填充目标bank，实际观察迁移触发GC后再回放各写擦边界。
            history = folder/'legacy-history.bin';history.write_bytes(b'\xff'*65536)
            upgrade_gc = None
            for rev in range(1,260):
                put_blob(history, 1, legacy(rev))
                if old_version==5:
                    padding=folder/'padding.dat';padding.write_bytes(bytes(64+(rev%7)*64))
                    run('putpad',history,1,padding)
                probe = folder/'migration-gc.bin';probe.write_bytes(history.read_bytes())
                old_gc = legacy(1000)
                put_blob(probe, 0, old_gc)
                snapshot = probe.read_bytes()
                result = run('migrate', probe, 0)
                if int(re.search(r'ERASES (\d+)', result.stdout)[1]):
                    upgrade_gc = ('migration-gc', snapshot, old_gc, raw_state(result), 0,
                                  int(re.search(r'OPS (\d+)', result.stdout)[1]))
                    break
            assert upgrade_gc is not None, '必须实际观察到迁移触发目标库GC'
            migration_cuts = 0
            for label, snapshot, old_disk, new_disk, bank, count in [migration_fixture, upgrade_gc]:
                probe = folder/'migration-cut.bin'
                for operation in range(1,count+1):
                    for prefix in range(5):
                        probe.write_bytes(snapshot)
                        run('migrate', probe, 0, operation, prefix, codes=(77,))
                        start = bank*32768
                        assert probe.read_bytes()[start:start+32768] == snapshot[start:start+32768]
                        recovered = run('boot', probe)
                        assert raw_state(recovered) in (old_disk,new_disk), (label,operation,prefix,recovered.stdout)
                        assert committed(recovered) == int.from_bytes(raw_state(recovered)[8:16], 'little')
                        migration_cuts += 1
                print(f'PASS: {label} {count} write/erase operations x5 cuts -> complete RAW old/v6, source unchanged', flush=True)
            all_migration_cuts += migration_cuts
            print(f'PASS: v{old_version} {migration_cuts} migration forced-process cuts', flush=True)

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
        # 随机地图身份与固定三关档案同事务，受控中断只能恢复完整旧/新快照。
        random_image=folder/'random.bin';random_image.write_bytes(b'\xff'*65536)
        random_archive=bytearray(state(run('init',random_image)))
        random_archive[513]=1
        struct.pack_into('<H',random_archive,514,1)
        struct.pack_into('<I',random_archive,516,70)
        random_archive=checksum(random_archive);put_blob(random_image,0,random_archive)
        before=random_image.read_bytes();old=state(run('boot',random_image));saved=run('save',random_image,1)
        new=state(saved);assert int.from_bytes(new[516:520],'little')==1
        fixtures.append(('random-map-identity',before,old,new,1,int(re.search(r'OPS (\d+)',saved.stdout)[1])))
        future_generator=folder/'future-generator.bin';future_generator.write_bytes(b'\xff'*65536)
        opaque=bytearray(random_archive);struct.pack_into('<H',opaque,514,99);opaque=checksum(opaque)
        put_blob(future_generator,0,opaque)
        assert state(run('save',future_generator,1))[513:520]==opaque[513:520]
        print('PASS: unknown generator v6 identity preserved in actual fixed-score save',flush=True)
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
        print(f'PASS: total {all_migration_cuts + cuts} forced-process cuts across v1/v2/v3/v4/v5 migration, ordinary/GC and initialization')
        print(json.dumps({'vendor_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                          for p in sources[-3:]}, 'command': command}))


if __name__ == '__main__':
    main()
