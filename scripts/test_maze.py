"""当前三固定关与随机娱乐模式的纯C/采样回归。"""
from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parent.parent
flags=['cc','-std=c99','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers','-I'+str(r/'src')]
with tempfile.TemporaryDirectory() as tmp:
 for harness,src in [
  ('archive_boundaries_harness',['maze_archive.c']),
  ('harness',['maze_app.c','maze_model.c','maze_level.c']),
  ('state_harness',['maze_app.c','maze_model.c','maze_level.c']),
  ('model_harness',['maze_model.c','maze_level.c','../tests/maze/legacy_levels.c']),
  ('inertia_harness',['maze_model.c','maze_level.c','../tests/maze/legacy_levels.c']),
  ('map_load_harness',['maze_map.c','maze_map_ice_loops.c']),
  ('third_map_harness',['maze_map.c','maze_model.c','maze_level.c','maze_map_ice_loops.c']),
  ('stage4_harness',['maze_app.c','maze_map.c','maze_model.c','maze_level.c','maze_map_mechanism_trial.c','maze_map_dynamic_trial.c']),
  ('trimmed_harness',['maze_archive.c','maze_progress.c','maze_catalog.c','maze_navigation.c','maze_app.c','maze_model.c','maze_level.c','maze_random.c','maze_random_templates.c','maze_map.c','maze_map_ice_loops.c','maze_map_mechanism_trial.c','maze_map_dynamic_trial.c'])]:
  exe=Path(tmp)/harness
  subprocess.run(flags+[str(r/'tests/maze'/(harness+'.c'))]+[str(r/'src'/s) for s in src]+['-lm','-o',str(exe)],check=True)
  subprocess.run([str(exe)],cwd=r,check=True)
 exe=Path(tmp)/'imu'
 subprocess.run(flags+['-I'+str(r/'tests/maze'),'-I'+str(r/'tests/imu_probe'),str(r/'tests/maze/imu_harness.c'),'-o',str(exe)],check=True)
 for mode in ['normal','late_read','stop_during_read','stop_during_status','read_failed','configure_failed','restore_failed','save_failed','bus_missing','not_ready','queued_failure','cadence','gap_latch']:subprocess.run([str(exe),mode],check=True)
for script in ['test_maze_dynamic.py','test_maze_random.py']:subprocess.run([sys.executable,str(r/'scripts'/script)],check=True)
