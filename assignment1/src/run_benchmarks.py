#!/usr/bin/python
"""
Usage: ./run_benchmarks.py launch [all] [cpu] [gpu] [memory] [disk] [network]
       ./run_benchmarks.py print [all] [cpu] [gpu] [memory] [disk] [network]
"""
import subprocess
import sys


def launch_benchmark(args):
    popen = subprocess.Popen(args, stdout=subprocess.PIPE)
    popen.wait()
    output = popen.stdout.read()
    sys.stdout.write(output)


def print_benchmark(args):
    print " ".join(args)


def cpu(func=launch_benchmark):
    """CPU benchmark"""
    FLOPS = "-f"
    IOPS = "-i"
    threads = map(str, [1, 2, 4, 8])
    op_types = (FLOPS, IOPS)
    operations = map(str, map(int, [1E7, 1E8, 1E9]))
    repeats = ["3"]

    headers = ["target", "op_type", "iteration", "n_operations",
               "n_threads", "time", "speed"]
    print ",".join(headers)

    for op_type in op_types:
        for n_t in threads:
            for n_op in operations:
                for r in repeats:
                    cpu_args = ["./cpu", op_type, "-o",
                                n_op, "-t", n_t, "-n", r]
                    func(cpu_args)


def gpu(func=launch_benchmark):
    """GPU benchmark"""
    # GPU speed benchmark
    FLOPS = "-f"
    IOPS = "-i"
    op_types = (FLOPS, IOPS)
    operations = map(str, map(int, [1E3, 1E4, 1E5]))
    repeats = ["3"]

    headers = ["target", "benchmark_type", "device", "op_type", "cores",
               "iterations", "n_repeats", "n_operations", "time",
               "speed (OPS/s)"]
    print ",".join(headers)

    for op_type in op_types:
        for n_op in operations:
            for r in repeats:
                gpu_args = ["./gpu", "-a", "speed", op_type, "-o",
                            n_op, "-n", r]
                func(gpu_args)

    # GPU bandwidth benchmark
    block_size = map(str, (1, 1024, 1024**2))

    headers = ["target", "benchmark_type", "device", "iterations", "n_repeats",
               "block_size", "n_operations", "time", "bandwidth (B/s)"]
    print ",".join(headers)

    for bs in block_size:
        for n_op in operations:
            for r in repeats:
                gpu_args = ["./gpu", "-a", "bandwidth", "-b", bs, "-o",
                            n_op, "-n", r]
                func(gpu_args)


def network(func=launch_benchmark):
    """Network benchmark"""
    TCP = 0
    UDP = 1
    THROUGHPUT = 0
    LATENCY = 1

    network_mode = map(str, (TCP, UDP))
    operations = map(str, map(int, [1E5, 1E6]))#[1E7, 1E8]))
    block_size = map(str, (1, 1024, 65536))
    threads = map(str, (1, 2))
    benchmark_type = map(str, (THROUGHPUT, LATENCY))

    headers = ["target", "network_mode", "n_operations", "n_threads",
               "block_sizes", "speed"]
    print ",".join(headers)

    for n_mode in network_mode:
        for n_t in threads:
            for n_op in operations:
                for b_size in block_size:
                    for b_type in benchmark_type:
                        network_args = ["./network", "-p", n_mode, "-o", n_op,
                                        "-b", b_size, "-t", n_t, "-l", b_type]
                        func(network_args)


def memory(func=launch_benchmark):
    """Memory benchmark"""
    SEQ = 0
    RAND = 1
    THROUGHPUT = 0
    LATENCY = 1

    memory_access = map(str, (SEQ, RAND))
    operations = map(str, map(int, [1E3, 1E4]))
    block_size = map(str, (1, 1024, 1048576))
    threads = map(str, (1, 2))
    benchmark_type = map(str, (THROUGHPUT, LATENCY))

    headers = ["target", "access_mode", "n_operations", "block_size",
               "n_threads", "speed"]
    print ",".join(headers)

    for m_access in memory_access:
        for n_t in threads:
            for n_op in operations:
                for b_size in block_size:
                    for b_type in benchmark_type:
                        memory_args = ["./memory", "-a", m_access, "-o", n_op,
                                       "-b", b_size, "-t", n_t, "-l", b_type]
                        func(memory_args)


def disk(func=launch_benchmark):
    threads = map(str, (1,2, 4))
    block_size = map(str, (1, 1024, 1024**2))  # 1B, 1KB, 1MB
    nb_operations = "10"
    global_repeats = ("3")

    for n_t in threads:
        # read|write * sequential|random
        for block in block_size:
            disk_args = ["./disk","-s", "-a", "-r", "-w", "-o", nb_operations, "-t", n_t,
                         "-n", global_repeats, "-b", block]
            func(disk_args)
        # latency
        disk_args = ["./disk", "-l", "-o", nb_operations, "-t", n_t, "-n",
                     global_repeats]
        func(disk_args)


if __name__ == "__main__":
    if len(sys.argv) == 1 or "-h" in sys.argv or "--help" in sys.argv:
        sys.stderr.write(__doc__)
        exit(1)

    benchmarks_func = [cpu, gpu, memory, disk, network]
    benchmarks = dict()
    for b in benchmarks_func:
        benchmarks[b.func_name] = b
    arg_funcs = {"launch": launch_benchmark,
                 "print": print_benchmark}

    benchmarks_asked = set(sys.argv).intersection(set(benchmarks.keys()))
    if "all" in sys.argv:
        benchmarks_asked = benchmarks.keys()
    if not benchmarks_asked:
        sys.stderr.write(__doc__)
        exit(1)
    arg_funcs_asked = set(sys.argv).intersection(set(arg_funcs.keys()))
    if not arg_funcs_asked:
        arg_funcs_asked = ["launch"]

    for b in benchmarks_asked:
        for af in arg_funcs_asked:
            benchmarks[b](arg_funcs[af])
