import argparse
import os

parser = argparse.ArgumentParser(description='Partition a dataset into several directories.')
parser.add_argument('-n', type=int, default='6', \
    help='number of partitions that dataset is devided into')
parser.add_argument('dirname', default='', help='path to the directory that contains dataset')
parser.add_argument('--merge', action='store_true', default=False,\
    help='merge the dataset')
args = parser.parse_args()


n_part = args.n
dirname = args.dirname

if not args.merge:
    files = os.listdir(dirname)
    n_files = len(files)
    n_each_part = n_files // n_part
    if n_each_part * n_part < n_files:
        n_each_part += 1

    for i in range(n_part):
        part_dir = os.path.join(dirname, str(i))
        os.makedirs(part_dir)
        start = n_each_part * i
        if i < n_part-1:
            end = n_each_part * (i+1)
        else: # i==n-1
            end = n_files
        for j in range(start, end):
            os.rename(os.path.join(dirname, files[j]), \
                os.path.join(part_dir, files[j]))
        
else:
    dir0 = os.path.join(dirname, str(0))
    for i in range(1, n_part):
        diri = os.path.join(dirname, str(i))
        files = os.listdir(diri)
        for f in files:
            os.rename(os.path.join(diri, f), os.path.join(dir0, f))
        os.removedirs(diri)
    files = os.listdir(dir0)
    for f in files:
        os.rename(os.path.join(dir0, f), os.path.join(dirname, f))
    os.removedirs(dir0)
        
