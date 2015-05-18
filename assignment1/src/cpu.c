#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "cpu.h"
#define CLOCKS_PER_MS (CLOCKS_PER_SEC/1000)

// Set the default values for the CLI arguments.
long N_OPERATIONS = 1E8;
int N_THREADS = 1;
int OP_TYPE = FLOPS;

void cpu_benchmark(float * timer) {
  pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t)*N_THREADS);
  int t_returns[N_THREADS];
  int i = 0;
  struct timeval start, end;
  long seconds, useconds;

  if (OP_TYPE != FLOPS && OP_TYPE != IOPS) {
    fprintf(stderr,
	    "Error: Wrong type argument in cpu_benchmark. Must be %d for CPU benchmark in term of FLOPS or %d for CPU benchmark in term of IOPS.\n",
	    FLOPS, IOPS);
    exit(1);
    return;
  }
  gettimeofday(&start, NULL); // starting timer
  for (i = 0; i < N_THREADS; i++) {
    if (OP_TYPE == FLOPS) { // calling float benchmark
      t_returns[i] = pthread_create(&threads[i], NULL,
				    cpu_flops, NULL);
    } else if (OP_TYPE == IOPS) {
      t_returns[i] = pthread_create(&threads[i], NULL,
				    cpu_iops, NULL);
    }
  }
  for (i = 0; i < N_THREADS; i++) {
    int ret = pthread_join(threads[i], NULL);
  }
  gettimeofday(&end, NULL); // end of the timer

  seconds = end.tv_sec - start.tv_sec;
  useconds = end.tv_usec - start.tv_usec;
  *timer = seconds + useconds/1000000.0;

  free(threads);
}

void* cpu_flops(void* arg) {
  int i = 0.;
  float nb = 0.;
  for (i = 0; i < N_OPERATIONS; i++) { // 2 operations: cmp and add
    // 27 operations: 9*(add, mul, move)
    nb = i*4.5321+1.2345;
    nb = i*453.21+123.45;
    nb = i*4532.1+1234.5;
    nb = i*6.789+9.876;
    nb = i*67.89+98.76;
    nb = i*678.9+987.6;
    nb = i*1.357+246.8;
    nb = i*13.57+24.68;
    nb = i*135.7+246.8;
  }
}

void* cpu_iops(void* arg){
  int i = 0;
  int nb = 0;
  for (i = 0; i < N_OPERATIONS; i++) { // 2 operations: cmp and add
    // 27 operations: 9*(add, mul, move)
    nb = i*453+123;
    nb = i*1234+5678;
    nb = i*9876+54321;
    nb = i*54321+12345;
    nb = i*67890+9876;
    nb = i*4321+7654;
    nb = i*13579+24680;
    nb = i*97531+86420;
    nb = i*102938+4756;
  }
}

int print_usage(char* argv[]) {
  fprintf(stderr, "Usage:\t%s [-f|-i] [-h] [-n N] [-o O] [-t T]\n", argv[0]);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "\t-f\tFlag for FLOPS benchmarking, used by default.\n");
  fprintf(stderr, "\t-h\tShow this help screen.\n");
  fprintf(stderr, "\t-i\tFlag for IOPS benchmarking.\n");
  fprintf(stderr, "\t-n N\tNumber of time the benchmark is repeated [default: 1].\n");
  fprintf(stderr, "\t-o O\tNumber of operations per loop [default: 1E8].\n");
  fprintf(stderr, "\t-t T\tNumber of threads to run the benchmark on [default: 1].\n");
}

int main (int argc, char *argv[]) {
  int n_repeats = 1;
  int c, i;

  // Parsing the command line
  while ((c = getopt(argc, argv, "fhin:o:t:")) != -1)
    switch (c) {
    case 'f':
      OP_TYPE = FLOPS;
      break;
    case 'h':
      print_usage(argv);
      exit(0);
    case 'i':
      OP_TYPE = IOPS;
      break;
    case 'n': // number of type we repeat the benchmark
      n_repeats = atoi(optarg);
      break;
    case 'o': // number of operations per loop
      N_OPERATIONS = (long)(atof(optarg)); // use atof to allow exponents
      break;
    case 't': // number of threads
      N_THREADS = atoi(optarg);
      break;
    case '?':
      print_usage(argv);
      return 1;
    default:
      abort ();
    }

  float times[n_repeats];// = (float *) malloc(n_repeats*sizeof(float));
  for (i = 0; i < n_repeats; i++) {
    cpu_benchmark(&times[i]);    
    printf("cpu,%s,%d,%ld,%d,%f,%f\n",
       OP_TYPE == FLOPS ? "FLOPS":"IOPS", i, N_OPERATIONS, N_THREADS,
       times[i], (N_THREADS*N_OPERATIONS*(27+2))/times[i]);
  }
}
