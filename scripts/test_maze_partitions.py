#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""核对项目表保留原区域及游戏独立空间；不连接设备。"""
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent
SDK=Path('D:/sifli/SiFli-SDK/v2.5.1')


def main():
    old=json.loads((SDK/'customer/boards/sf32lb52-lchspi-ulp/ptab.json').read_text(encoding='utf-8'))
    new=json.loads((ROOT/'project/ptab.json').read_text(encoding='utf-8'))
    assert len(old)==len(new)
    for a,b in zip(old,new):
        assert a['mem']==b['mem'] and a['base']==b['base']
        assert b['regions'][:len(a['regions'])]==a['regions']
    bank=new[0]
    proposed=bank['regions'][-2:]
    for i,region in enumerate(proposed):
        assert int(region['offset'],0)==0xdb0000+i*32768
        assert int(region['max_size'],0)==32768
        assert region['tags']==[f'MAZE_SAVE_{"AB"[i]}_REGION']
        assert 'img' not in region and 'ftab' not in region
    regions=sorted((int(x['offset'],0),int(x['max_size'],0)) for x in bank['regions'])
    assert all(x+n<=y for (x,n),(y,_) in zip(regions,regions[1:]))
    assert regions[-1][0]+regions[-1][1]<=16*1024*1024
    config=(ROOT/'project/proj.conf').read_text(encoding='utf-8')
    for setting in ['CONFIG_PKG_USING_FLASHDB=y','CONFIG_PKG_FDB_USING_FAL_MODE=y',
                    'CONFIG_PKG_FLASHDB_ERASE_GRAN=8192']:
        assert setting in config
    print('PASS: original partitions unchanged; aligned A/B 32KiB and 8KiB erase configuration, no save image')


if __name__=='__main__':
    main()
