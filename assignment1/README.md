# Compilation

To compile this project, start by cloning this repository. Then, go in the folder `src` and use `make` to compile all the executables. You should then have five new executables:

- cpu
- memory
- network
- disk
- gpu

This Makefile has been successfully tested under the following configurations:

- gcc 4.4.7, 4.8.3, 4.8.1
- nvcc 5.5.0

Next, we describe how to use these executables.

# Command Line Interface

In this manual, we show how to use the command line interfaces (CLI) we have developed. For each CLI, possible options are explained and examples are given.

## CPU

###### Developed by Virgile LANDEIRO DOS REIS

### Usage

```
./cpu [-f|-i] [-h] [-n N] [-o O] [-t T]
```

### Options

```
-f      Flag for FLOPS benchmarking, used by default.
-h      Show this help screen.
-i      Flag for IOPS benchmarking.
-n N    Number of time the benchmark is repeated [default: 1].
-o O    Number of operations per loop [default: 1E8].
-t T    Number of threads to run the benchmark on [default: 1].
```

### Examples

The simpliest way to run this CLI is by giving 0 argument:

```
./cpu
```

This command runs the CPU benchmark with all the default parameter. Therefore, the FLOPS benchmark will be executed once, on 100,000,000 operations and on a unique thread.

If one wants to run the CPU benchmark to compute IOPS on 2 threads, 100,000 operations, and to repeat it 3 times to be able to compute an average executing time, one can use the following command:

```
./cpu -i -o 1E5 -n 3 -t 2
```

## GPU

###### Developed by Virgile LANDEIRO DOS REIS

### Usage

The GPU benchmark has two modes, one to benchmark the GPU speed (in FLOPS or IOPS) and one to measure the bandwidth of the GPU memory.

```
./gpu -a speed [-f|-i] [-h] [-n N] [-o O]
./gpu -a bandwidth [-b B] [-n N] [-o O]
```

### Options

```
-b B    Size of the block to be allocated in memory [default:1024].
-f      Flag for FLOPS benchmarking, used by default.
-h      Show this help screen.
-i      Flag for IOPS benchmarking.
-n N    Number of time the benchmark is repeated [default: 1].
-o O    Number of operations per loop [default: 1E5].
```

### Examples

To launch the benchmarks with the default value, just use the following commands:

```
./gpu -a speed
./gpu -a bandwidth
```

If one wants to benchmark the GPU on floating point operations, doing 1,000,000 operations and repeating the benchmark 5 times, one can use:

```
./gpu -a speed -i -o 1E6 -n 5
```

If one wants to run the memory bandwidth benchmark with a block size of 1MB, repeating this benchmark 3 times on 1,000 operations, one uses this command:

```
./gpu -a bandwidth -b 1048576 -o 1000 -n 3
```

## Memory

###### Developed by Tony FORLINI

### Usage

The memory benchmark has two modes, one to benchmark random access memory and one to measure sequential access memory.

```
./memory -a sequential [-o 0] [-b B] [-t T]  [-l L]
./memory -a random [-o 0] [-b B] [-t T]  [-l L]
```

### Options

```
-a	Flag for Sequential or random access to memory SEQ=0 & RAND=1
-h 	Show the help screen
-o 	Number of operation per loop
-b 	Number of bytes in block 
-t 	Number of threads to run the benchmark on 
-l 	Flag for latency or throughput LTC=1 & THRPT=0
```

### Examples

The simpliest way to run this CLI is by giving 0 argument:

```
./memory
```

This command runs the memory benchmark with all the default parameter. 

If one wants to run the memory benchmark to compute latency with 2 threadsv for sequential access , with 1KB block, and to repeat it 1000 times to be able to compute an average executing time, one can use the following command:

```
./memory -a 0 -o 1000 -b 1024 -t 2 -l 1

```

## Disk

###### Developed by Thomas DUBUCQ

### Usage

The disk benchmark has 5 modes : latency, reading, writing, sequential and random.

The basic execution of this benchmark should use one of the following combinaison of modes

    ./disk -l [-b B] [-o O] [-n N] [-t T]    for latency
    ./disk -s -w [-b B] [-o O] [-n N] [-t T] for sequential writing
    ./disk -s -r [-b B] [-o O] [-n N] [-t T] for sequential reading
    ./disk -a -w [-b B] [-o O] [-n N] [-t T] for random writing
    ./disk -a -r [-b B] [-o O] [-n N] [-t T] for random reading
    
### Options

```
-a  Flag for random benchmarking
-b  Block size in Bytes [default: 1kB]
-h  Show help screen
-l  Flag for latency benchmarking
-o  Number of operations per loop [default: 1000].
        This is the number of operations that will be executed in a single benchmark timing.
        The result returned will then be the average length of one operation.
-n  Number of time the benchmark is repeated [default: 1]
-r  Flag for read benchmarking
-s  Flag for sequential benchmarking
-t  Number of threads to run the benchmark on [default: 1]
-w  Flag for write benchmarking
```

### Examples

The simpliest way to run this CLI is by giving 0 argument:

```
./disk
```

This command runs the network benchmark with all the default parameters. 

Using multiple flags will launch benchmarking for all possible combinaisons of flags.
    Ex :  `./disk -l -w -s -a` will compute latency, sequential writing and random writing
    

## Network
 
###### Developed by Tony FORLINI

### Usage


The network benchmark has two modes, one to benchmark TCP protocol and one to measure the UDP protocol.

```
./network -p TCP [-o 0] [-b B] [-t T]  [-l L]
./network -p UDP [-o 0] [-b B] [-t T]  [-l L]

```

### Options

```
-p	Flag for TCP or UDP protocol: TCP=0 & UDP=1
-h 	Show the help screen
-o 	Number of operation per loop 
-b 	Number of bytes in block 
-t 	Number of threads to run the benchmark on 
-l 	Flag for latency or throughput LTC=1 & THRPT=0
```

### Examples

The simpliest way to run this CLI is by giving 0 argument:

```
./network
```

This command runs the network benchmark with all the default parameter. 

If one wants to run the network benchmark to compute throughput with 1 thread for TCP protocol , with 1B block, and to repeat it 1000 times to be able to compute an average executing time, one can use the following command:

```
./network -p 0 -o 1000 -b 1 -t 1 -l 0

```

## Benchmark Runner

###### Developed by Virgile LANDEIRO DOS REIS

### Usage

In order to automatize all the benchmarks, we have created a benchmark runner. It is a Python script that has the following usage:

```
./run_benchmarks.py launch [all] [cpu] [gpu] [memory] [disk] [network]
./run_benchmarks.py print [all] [cpu] [gpu] [memory] [disk] [network]
```

### Examples

Using this script, it is possible to launch all the benchmarks we have developed once the executables have been built. To do so, one can use the following command:

```
./run_benchmarks.py launch all
```

Now if the user only wants the memory and disk benchmarks, he can use:

```
./run_benchmarks.py launch memory disk
```

The `print` argument allows the user to print all the commands that are going to be executed on the system. If the user wants to know what are the commands running on the system when he is doing a GPU benchmark, he can do:

```
./run_benchmarks.py print gpu
```
