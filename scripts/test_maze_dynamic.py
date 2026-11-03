"""编译真实动态模型与地图，验证第5阶段。"""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parent.parent
with tempfile.TemporaryDirectory() as d:
    exe=Path(d)/'dynamic'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers',
        '-I'+str(root/'src'),str(root/'tests/maze/dynamic_harness.c'),
        *[str(root/'src'/s) for s in ['maze_model.c','maze_level.c','maze_app.c']],'-lm','-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)

    route=Path(d)/'route'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers',
        '-I'+str(root/'src'),str(root/'tests/maze/stage5_route_harness.c'),
        *[str(root/'src'/s) for s in ['maze_model.c','maze_level.c','maze_map.c','maze_map_dynamic_trial.c']],'-lm','-o',str(route)],check=True)
    subprocess.run([str(route)],check=True)
    cost=Path(d)/'cost'
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers',
        '-finstrument-functions','-I'+str(root/'src'),str(root/'tests/maze/dynamic_cost_harness.c'),
        *[str(root/'src'/s) for s in ['maze_map.c','maze_level.c','maze_map_dynamic_trial.c']],'-lm','-o',str(cost)],check=True)
    subprocess.run([str(cost)],check=True)
