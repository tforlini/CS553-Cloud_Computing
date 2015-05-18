#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#define FLOPS   1
#define IOPS    2

#define WRAP_MEM_ALLOC(W_N,W_TYPE)					\
  a = malloc(sizeof(W_TYPE)*W_N);					\
  cudaMalloc(&d_a,W_N*sizeof(W_TYPE));					\
  a##W_TYPE = (W_TYPE *) a;						\
  for (i = 0; i < W_N; i++) {a##W_TYPE[i] = i; }

#define CUDA_BENCHMARK(CB_FUNC,CB_BLOCK,CB_THREAD,CB_TYPE,CB_NOPS)	\
  cudaEventRecord(start);						\
  CB_FUNC<<<CB_BLOCK,CB_THREAD>>>((CB_TYPE)d_a,cores_count,CB_NOPS);	\
  cudaEventRecord(stop);						\
  cudaEventSynchronize(stop);						\
  cudaEventElapsedTime(&elapsedTime, start, stop);

int 
gcd ( int a, int b )
{
  int c;
  while ( a != 0 ) {
    c = a; a = b%a;  b = c;
  }
  return b;
}


__global__ void VectorOpsInt(int * a, int n, int n_ops) {
  /* int i = threadIdx.x; */
  int i = blockIdx.x*blockDim.x+threadIdx.x;
  int j = 0;

  if (i < n) {
    for (j = 0; j < n_ops; j++) { // 2 operations
      // 3 ops by line: mul, add, and mv
      a[i] = a[i]*453+123;
      a[i] = a[i]*1234+5678;
      a[i] = a[i]*9876+54321;
      a[i] = a[i]*54321+12345;
      a[i] = a[i]*67890+9876;
      a[i] = a[i]*4321+7654;
      a[i] = a[i]*13579+24680;
      a[i] = a[i]*97531+86420;
      a[i] = a[i]*102938+4756;
    }
  }
}

__global__ void VectorOpsFloat(float * a, int n, int n_ops) {
  /* int i = threadIdx.x; */
  int i = blockIdx.x*blockDim.x+threadIdx.x;
  int j = 0;

  if (i < n) {
    for (j = 0; j < n_ops; j++) { // 2 operations
      // 3 ops by line: mul, add, and mv
      a[i] = a[i]*4.5321+1.2345;
      a[i] = a[i]*453.21+123.45;
      a[i] = a[i]*4532.1+1234.5;
      a[i] = a[i]*6.789+9.876;
      a[i] = a[i]*67.89+98.76;
      a[i] = a[i]*678.9+987.6;
      a[i] = a[i]*1.357+246.8;
      a[i] = a[i]*13.57+24.68;
      a[i] = a[i]*135.7+246.8;
    }
  }
}

int computeConfiguration(int* config) {
  /**
     Gets the device properties to know the maximal number of threads we can
     get running on the GPU and how to organize them in blocks.
     Returns an integer array A of size 2 where:
     - A[0] = number of blocks
     - A[1] = number of threads per block
  **/
  cudaDeviceProp devProp;
  cudaGetDeviceProperties(&devProp, 0);
  int n_proc = devProp.multiProcessorCount;
  int threads_per_proc = devProp.maxThreadsPerBlock;
  int threads_per_block = devProp.maxThreadsPerBlock;
  int max_threads = n_proc*threads_per_proc;

  /* Compute the greatest common denominator of the maximum number of threads we can run on the GPU and the number of threads we can run per block. Therefore, we can homogeneously distribute threads over blocks.*/
  int n_blocks = gcd(max_threads, threads_per_block);
  int n_threads = max_threads/n_blocks;

  config[0] = n_blocks;
  config[1] = n_threads;
  return max_threads;
}

int getCoresCount() {
  /**
     Gets the device number of cores.
  **/
  cudaDeviceProp devProp;
  cudaGetDeviceProperties(&devProp, 0);
  int n_proc = devProp.multiProcessorCount;
  return n_proc;
}

float gpu_speed_benchmark(int type, int n_operations) {
  void *a = NULL;
  void *d_a = NULL;
  float elapsedTime = 0;
  int config[2] = {0};

  // Initialize cuda timer
  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  int cores_count = computeConfiguration(config);// getCoresCount();

  int i = 0;
  float * afloat = NULL;
  int * aint = NULL;

  // Allocate memory on host and on device.
  switch (type) {
  case FLOPS:
    WRAP_MEM_ALLOC(cores_count,float)
      cudaMemcpy(d_a, a, cores_count*sizeof(float), cudaMemcpyHostToDevice);
      break;
  case IOPS:
    WRAP_MEM_ALLOC(cores_count,int)
      cudaMemcpy(d_a, a, cores_count*sizeof(int), cudaMemcpyHostToDevice);
      break;
  default:
    break;
  }

  // Launch benchmark
  switch(type) {
  case IOPS:
    CUDA_BENCHMARK(VectorOpsInt,config[0],config[1],int*,n_operations)
      /* CUDA_BENCHMARK(VectorOpsInt,1,cores_count,int*,n_operations) */
      /* Copy the result to the host */
      cudaMemcpy(a, d_a, cores_count*sizeof(int), cudaMemcpyDeviceToHost);
    break;
  case FLOPS:
    CUDA_BENCHMARK(VectorOpsFloat,config[0],config[1],float*,n_operations)
      /* CUDA_BENCHMARK(VectorOpsFloat,1,cores_count,float*,n_operations) */
      /* Copy the result to the host */
      cudaMemcpy(a, d_a, cores_count*sizeof(float), cudaMemcpyDeviceToHost);
    break;
  default:
    break;
  }

  free(a);
  cudaFree(d_a);
  return elapsedTime;
}

float gpu_bandwidth_benchmark(int block_size, int n_operations) {
  int i = 0;
  float elapsedTime = 0;
  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);
  void * a = NULL;
  void * d_a = NULL;
  void * d_b = NULL;
  char * achar = NULL;

  WRAP_MEM_ALLOC(block_size,char)
  cudaMalloc(&d_b, block_size*sizeof(char));
  cudaMemcpy(d_a, a, block_size*sizeof(char), cudaMemcpyHostToDevice); 

  cudaEventRecord(start);
  for (i = 0; i < n_operations; i++) {
    cudaMemcpy(d_b, d_a, block_size, cudaMemcpyDeviceToDevice); 
  }
  cudaEventRecord(stop);
  cudaEventSynchronize(stop);

  cudaEventElapsedTime(&elapsedTime, start, stop);  

  free(a);
  cudaFree(d_a);
  return elapsedTime;;
}

void print_usage(char* argv[]) {
  fprintf(stderr, "Usage:\t%s -a speed [-f|-i] [-h] [-n N] [-o O]\n", argv[0]);
  fprintf(stderr, "\t%s -a bandwidth [-b B] [-n N] [-o O]\n", argv[0]);
  fprintf(stderr, "\nOptions: \n");
  fprintf(stderr, "\t-b B\tSize of the block to be allocated in memory [default:1024].\n");
  fprintf(stderr, "\t-f\tFlag for FLOPS benchmarking, used by default.\n");
  fprintf(stderr, "\t-h\tShow this help screen.\n");
  fprintf(stderr, "\t-i\tFlag for IOPS benchmarking.\n");
  fprintf(stderr, "\t-n N\tNumber of time the benchmark is repeated [default: 1].\n");
  fprintf(stderr, "\t-o O\tNumber of operations per loop [default: 1E5].\n");
}

int main(int argc, char* argv[]) {
  int n_repeats = 1;
  long n_operations = 10000;
  int type = FLOPS;
  long block_size = 1024;
  int c, i;
  float elapsedTime = 0;
  char * a_value = NULL;
  cudaDeviceProp dev;
  cudaGetDeviceProperties(&dev, 0);

  // Parsing the command line
  while ((c = getopt(argc, argv, "fhia:b:n:o:")) != -1)
    switch (c) {
    case 'a':
      a_value = optarg;
      if (strcmp(a_value, "speed") and strcmp(a_value, "bandwidth")) {
        print_usage(argv);
        exit(1);
      }
      break;
    case 'b':
      block_size = (long)(atof(optarg)); // use atof to allow exponents
      break;
    case 'f':
      type = FLOPS;
      break;
    case 'h':
      print_usage(argv);
      exit(0);
    case 'i':
      type = IOPS;
      break;
    case 'n': // number of type we repeat the benchmark
      n_repeats = atoi(optarg);
      break;
    case 'o': // number of operations per loop
      n_operations = (long)(atof(optarg)); // use atof to allow exponents
      break;
    case '?':
      print_usage(argv);
      return 1;
    default:
      abort ();
    }

  int config[2] = {0};
  int n_cores = computeConfiguration(config); //getCoresCount();
  if (strcmp(a_value, "speed") == 0) {
    for (i = 0; i < n_repeats; i++) {
      elapsedTime = gpu_speed_benchmark(type, n_operations)/1000;
      float speed = ((27+2)*n_operations*n_cores)/elapsedTime;
      printf("gpu,speed,%s,%s,%d,%d,%d,%d,%f,%f\n",
             dev.name, type == FLOPS ? "FLOPS":"IOPS",
	     n_cores,i, n_repeats, n_operations, elapsedTime, speed);
    }
  } else if (strcmp(a_value, "bandwidth") == 0) {
    for (i = 0; i < n_repeats; i++) {
      elapsedTime = gpu_bandwidth_benchmark(block_size, n_operations)/1000;
      float bandwidth = (block_size*n_operations)/elapsedTime;
      printf("gpu,bandwidth,%s,%d,%d,%d,%d,%f,%f\n",
             dev.name, i, n_repeats, block_size, n_operations, elapsedTime, bandwidth);
    }
  }
}
