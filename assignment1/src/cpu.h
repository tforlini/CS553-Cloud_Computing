#ifndef _CPU_H_
#define _CPU_H_

#define FLOPS   1
#define IOPS    2

void cpu_benchmark(float *);
void * cpu_flops(void *);
void * cpu_iops(void *);
int print_usage(char* argv[]);
int main(int argc, char* argv[]);

#endif // _CPU_H_
