from pathlib import Path
import os

os.makedirs('testdir')
for i in range(1000):
    Path('testdir/test{:03}.txt'.format(i)).touch()
