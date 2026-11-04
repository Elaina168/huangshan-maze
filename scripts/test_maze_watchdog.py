"""实际WDT适配器与SDK头文件的主机契约，不代替物理超时复位。"""
import argparse,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--sdk',type=Path,required=True);a=p.parse_args()
r=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory() as t:
 exe=Path(t)/'watchdog'
 subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',*['-I'+str(r/x) for x in ['tests/maze/main-host','tests/maze','src']],'-I'+str(a.sdk/'rtos/rtthread/components/drivers/include'),str(r/'tests/maze/watchdog_harness.c'),str(r/'src/maze_watchdog.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
