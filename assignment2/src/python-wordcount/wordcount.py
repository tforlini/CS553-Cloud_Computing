"""
Usage: wordcount.py <n_shards> <n_processes> <filepath>
"""
#from threading import Thread, Lock
from multiprocessing import Process, Lock, Manager, Queue
import sys
import os.path
import math
import re
import string
from docopt import docopt
from collections import defaultdict

punc = ",;.|'"
punctuation_regex = re.compile('[%s]' % re.escape(punc))

def split_file(filepath, n):
    """
    This function breaks a given file in n shards while avoiding to cut words.
    *Inputs*
    - filepath [str] path to the file to split in pieces
    - n [int] number of shards to generate
    - q = [(offset0, size), (offset1, size), ..., (offsetn, size)]: a queue of n tuples. The ith tuple holds the offset to access the ith shard in the input file and the size of the shard.
    """
    fs = os.path.getsize(filepath)
    fo = open(filepath, "r")
    size_shard = int(math.ceil(fs/float(n)))
    q = Queue()
    cumul_offset = 0
    c = ''
    for i in range(n):
        sh_i_offset = cumul_offset
        sh_i_size = min(size_shard, int(fs-cumul_offset))
        fo.seek(sh_i_offset + sh_i_size)
        while True and i != n-1:
            c = fo.read(1)
            sh_i_size += 1
            if c == ' ' or c == '\n':
                break
        q.put((sh_i_offset, sh_i_size))
        cumul_offset += sh_i_size
    fo.close()
    return q

def check_L(filepath, L):
    """
    Prints the 10 first and 10 last characters of each block defined by split_file. This function is used to check that split_file works correctly.
    """
    print sum(t[1] for t in L)
    with open(filepath, "r") as fo:
        for t in L:
            fo.seek(t[0])
            shard = fo.read(t[1])

def wordcount(fp, shards, results):
    """
    Reports the frequency of each word in a shard in a counter.
    """
    with open(fp) as fo:
        counts = defaultdict(int)
        while not shards.empty():
            offset, size = shards.get()
            # sys.stderr.write(str((offset,size)) + "\n")
            fo.seek(offset)
            shard = fo.read(size)
            shard_wo_punc = punctuation_regex.sub('', shard)
            lines = shard_wo_punc.splitlines()
            for line in lines:
                words = [w.strip() for w in line.split()]
                for w in words:
                    counts[w] += 1
        results.append(counts)

def main(args):
    n_shards = int(args['<n_shards>'])
    n_processes = min(n_shards, int(args['<n_processes>']))
    fp = args['<filepath>']

    manager = Manager()
    shards = split_file(fp, n_shards)
    shards_mutex = Lock()
    results_q = Queue()

    m_results = manager.list() #defaultdict(int)

    processes = []
    for p_i in range(n_processes):
        p = Process(target=wordcount,
                    args=(fp, shards, m_results))
        #p.daemon = True
        p.start()
        processes.append(p)
    for p in processes:
        p.join()

    # outputs the result to stdin
    cumulated_results = defaultdict(int)

    for d in m_results:
        for k in d.keys():
            cumulated_results[k] += d[k]

    for k in cumulated_results.keys():
        print str(cumulated_results[k]) + "\t" + k
    return 0

if __name__ == "__main__":
    args = docopt(__doc__)
    main(args)