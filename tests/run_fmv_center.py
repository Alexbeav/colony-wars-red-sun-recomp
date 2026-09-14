"""Compile and run the retail-free movie slice hook checks."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--framework', type=Path, required=True)
args = p.parse_args()
project = Path(__file__).resolve().parents[1]
output = project / '_scratch/tests/fmv_center'
output.mkdir(parents=True, exist_ok=True)
cc = os.environ.get('CC') or shutil.which('gcc') or shutil.which('clang')
if not cc:
    p.error('Set CC or put gcc/clang on PATH.')
exe = output / ('test_fmv_center.exe' if os.name == 'nt' else 'test_fmv_center')
subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-I' + str(args.framework / 'runtime/include'),
                str(project / 'tests/test_fmv_center.c'), '-o', str(exe)], check=True)
subprocess.run([str(exe)], check=True)
