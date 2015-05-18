"""
Usage: dataset_generator.py <output_dir>
"""
from docopt import docopt
import string
import random
import os.path

size_dict = {"1kb": 1024,
			 "10kb": 10*1024,
			 "100kb": 100*1024,
			 "1mb": 1024**2,
			 "10mb": 10*1024**2,
			 "100mb": 100*1024**2}

def gen_rand_str(size=10, alphabet=string.ascii_lowercase+string.digits):
    return ''.join(random.choice(alphabet) for i in range(size))

def gen_rand_file(size="1kb", linesize=100, output_dir="./"):
    if type(size) is list: # generate multiple files with one instruction
        for s in size:     # by calling recursively the generation function
            gen_rand_file(s, linesize, output_dir)

    if type(size) is str:  # parse the text size
        size = size_dict[size.lower()]

    if type(size) is int:  # generate one file
        filename = gen_rand_str()
        filepath = os.path.join(output_dir, filename)
        with open(filepath, "w+") as f:
            while size > 0:
            	linesize = min(linesize, size)
                rand_str = gen_rand_str(linesize-2,
                                        string.letters+string.digits)
                f.write(rand_str + '\n')
                size -= linesize

def gen_dataset(dataset_desc, output_dir):
    for size in dataset_desc.keys():
        nb = dataset_desc[size]
        gen_rand_file([size]*nb, 100, output_dir)

def main(args):
    dataset = {
                "1KB": 100,
                "10KB": 100,
                "100KB": 100,
                "1MB": 100,
                "10MB": 10,
                "100MB": 1
              }
    gen_dataset(dataset, args['<output_dir>'])

if __name__ == "__main__":
    args = docopt(__doc__)
    main(args)