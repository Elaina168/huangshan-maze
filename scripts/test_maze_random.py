"""真实地图状态机、生成器与随机机制回归。"""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parent.parent
flags=['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers','-I'+str(root/'src')]
with tempfile.TemporaryDirectory(prefix='maze-random-') as tmp:
 for name,sources in [('random_map_step_harness',['maze_map.c','maze_map_ice_loops.c','maze_map_mechanism_trial.c','maze_map_dynamic_trial.c']),('random_generator_harness',['maze_random.c','maze_random_templates.c','maze_map.c']),
 ('random_route_harness',['maze_random.c','maze_random_templates.c','maze_map.c','maze_model.c','maze_level.c'])]:
  exe=Path(tmp)/name
  subprocess.run(flags+[str(root/'tests/maze'/(name+'.c'))]+[str(root/'src'/s) for s in sources]+['-lm','-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
