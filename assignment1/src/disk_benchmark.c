/*
 * disk_benchmark.c
 *
 *  Created on: Sep 15, 2014
 *      Author: TDubucq
 */

#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<time.h>
#include<sys/types.h>
#include<pthread.h>
#include<string.h>
#include<sys/time.h>
typedef struct timeval timeval;
#define MAX_THREAD 10

struct timezone tz = {0,0};



void sequential_writing( clock_t* result, int disk_block_size, FILE* fp, int n_lrepeat){
	int i;
	clock_t t0,t1;
	int fd = fileno(fp);
	char* cha = (char*)calloc(disk_block_size*sizeof(char),1);

	t0 = clock();
	for(i = 0; i < n_lrepeat; i++){
		write(fd,cha,disk_block_size);
		sync();
	}
	fclose(fp);
	*result = clock() - t0;


	free(cha);
}

void sequential_reading (clock_t* result, int disk_block_size, FILE* fp, int n_lrepeat){
	int i;
	clock_t t0,t1;
	int fd = fileno(fp);

	//writing on disk
	char* cha = (char*)calloc(disk_block_size*sizeof(char),1);
	char * read_c  = (char*) malloc (disk_block_size*sizeof(char));
	fseek(fp,0,SEEK_SET);
	fwrite(cha,disk_block_size*sizeof(char),1,fp);
	fflush(fp);
	fsync(fd);

	//benchmark
	t0 = clock();
	for(i = 0; i < n_lrepeat; i++){
		fread(read_c,disk_block_size*sizeof(char),1,fp);
		fflush(fp);
	}
	*result = clock() - t0;
	fclose(fp);


	free(read_c);
	free(cha);

}

void random_writing( clock_t* result, int disk_block_size, FILE* fp, int n_lrepeat){
	int i,j;
	clock_t t0;
	int fd = fileno(fp);
	char* cha0 = (char*)calloc(disk_block_size*sizeof(char),0);
	char* cha = (char*)calloc(disk_block_size*sizeof(char),1);
	t0 = clock();

	//Initialisation of the random starting points for the writing
	int* headers = (int*) malloc(n_lrepeat*sizeof(int));
	for(i = 0; i < n_lrepeat; i++){
		headers[i] =  rand() % (n_lrepeat*disk_block_size);
	}
	// File initialisation
	for(i = 0; i < n_lrepeat; i++){
		fwrite(cha0,disk_block_size*sizeof(char), 1,fp);
	}
	fsync(fd);
	fflush(fp);
	//Benchmark
	t0 = clock();
	for(j = 0; j < n_lrepeat; j++){
		for(i = 0; i < n_lrepeat; i++){
			fseek(fp,headers[i],SEEK_SET);
			fwrite(cha,disk_block_size*sizeof(char), 1,fp);
			fsync(fd);
			fflush(fp);
		}
	}
	fclose(fp);
	*result = clock() - t0;

	free(cha);
	free(headers);

}

void random_reading (clock_t* result, int disk_block_size, FILE* fp, int n_lrepeat){
	int i,j;
	clock_t t0;
	int fd = fileno(fp);

	char* cha = (char*)calloc(disk_block_size*sizeof(char),1);

	fseek(fp,0,SEEK_SET);
	for(i = 0; i < n_lrepeat; i++){
		fwrite(cha,disk_block_size*sizeof(char),1,fp);
	}
	fflush(fp);
	fsync(fd);

	//Initialisation of the random starting points for the writing
	int* headers = (int*) malloc(n_lrepeat*sizeof(int));
	for(i = 0; i < n_lrepeat; i++){
		headers[i] =  rand() % (n_lrepeat*disk_block_size);
	}

	//Benchmark
	t0 = clock();
	for(j = 0; j < n_lrepeat; j++){
		for(i = 0; i < n_lrepeat; i++){
			fseek(fp,headers[i],SEEK_SET);
			fread(cha,disk_block_size*sizeof(char), 1,fp);
			fflush(fp);
		}
	}
	*result = clock() - t0;
	fclose(fp);
	free(headers);
	free(cha);
}

void latency (clock_t* result, int block_size, FILE* fp, int n_lrepeat){
	int i;
	clock_t t0;
	int fd = fileno(fp);
	int* headers = (int*) malloc(n_lrepeat*sizeof(int));
	for(i = 0; i < n_lrepeat; i++){
		headers[i] = (block_size * rand()) % block_size; //homogenous distribution of possible writing starting points
	}

	char* cha = (char*)calloc(block_size*sizeof(char),1);
	char* tmp = (char*)calloc(sizeof(char),0);

	fseek(fp,0,SEEK_SET);
	fwrite(cha, block_size*sizeof(char),block_size,fp);
	fflush(fp);
	fsync(fd);

	//Benchmark
	t0 = clock();
	for(i = 0; i < n_lrepeat; i++){
		fseek(fp,headers[i],SEEK_SET);
		fwrite(tmp,sizeof(char),1,fp);
		fflush(fp);
		fsync(fd);
	}
	*result = clock() - t0;
	fclose(fp);
	free(headers);
	free(cha);
	free(tmp);
}

typedef struct {unsigned int latency:1; unsigned int sequential:1; unsigned int random:1;
unsigned int read:1; unsigned int write:1;unsigned int nb_threads:27;unsigned int n_brepeat;
unsigned int n_lrepeat; unsigned int block_size;
pthread_mutex_t* fmutex_p; FILE * results_fp;} bmrk_arg;

void output(pthread_mutex_t* fmutex_p, FILE * results_fp, char*output_p){
	size_t size = strlen(output_p);
	printf("output: output_p =\n%s",output_p);
	pthread_mutex_lock(fmutex_p);
	fwrite(output_p, size, 1, results_fp);
	pthread_mutex_unlock(fmutex_p);
}

float clockstoMBpS(int block_size, clock_t result, int n_lrepeat){
	return n_lrepeat*block_size*CLOCKS_PER_SEC/(float)result/1045876;
}
//XXX only allows 10 threads
void* bmrk_launcher(void* arg){
	bmrk_arg * arg_p = (bmrk_arg*) arg;
	clock_t result;
	int i;
	float finalresult,time;
	char* output_s = (char*) malloc(100*sizeof(char));
	char* tmp_file_name = (char*) malloc(24*sizeof(char));
	sprintf(tmp_file_name, "tmp_diskbenchmark%d.txt", rand()%10);
	FILE * fp;

	for(i = 0; i < arg_p->n_brepeat; i++){
		if(arg_p->latency){
			fp = fopen(tmp_file_name, "w+") ;
			latency (&result, arg_p->block_size, fp, arg_p->n_lrepeat);
			finalresult = (float)result/(CLOCKS_PER_SEC/1000)/arg_p->n_lrepeat;
			sprintf(output_s, "disk,%d, %d, %d, latency, , %fms\n", arg_p->nb_threads,arg_p->block_size, arg_p->n_lrepeat, finalresult);
			output(arg_p->fmutex_p,arg_p->results_fp,output_s);
		}
		if(arg_p->sequential){
			if(arg_p->read){
				fp = fopen(tmp_file_name, "w+") ;
				sequential_reading(&result, arg_p->block_size, fp, arg_p->n_lrepeat);
				time = (float)result/(CLOCKS_PER_SEC/1000)/arg_p->n_lrepeat;
				finalresult = clockstoMBpS(arg_p->block_size, result, arg_p->n_lrepeat);
				sprintf(output_s, "disk, %d, %d, %d, read, sequential, %fms, %fMB/s\n", arg_p->nb_threads, arg_p->block_size, arg_p->n_lrepeat, time, finalresult);
				output(arg_p->fmutex_p,arg_p->results_fp,output_s);
			}
			if(arg_p->write){
				fp = fopen(tmp_file_name, "w+") ;
				sequential_writing(&result, arg_p->block_size, fp, arg_p->n_lrepeat);
				time = (float)result/(CLOCKS_PER_SEC/1000)/arg_p->n_lrepeat;
				finalresult = clockstoMBpS(arg_p->block_size, result, arg_p->n_lrepeat);
				sprintf(output_s, "disk, %d, %d, %d, write, sequential, %fms, %fMB/s\n", arg_p->nb_threads, arg_p->block_size, arg_p->n_lrepeat, time, finalresult);
				output(arg_p->fmutex_p,arg_p->results_fp,output_s);
			}
		}
		if(arg_p->random){
			if(arg_p->read){
				fp = fopen(tmp_file_name, "w+") ;
				random_reading(&result, arg_p->block_size, fp, arg_p->n_lrepeat);
				time = (float)result/(CLOCKS_PER_SEC/1000)/arg_p->n_lrepeat;
				finalresult = clockstoMBpS(arg_p->block_size, result, arg_p->n_lrepeat);
				sprintf(output_s, "disk, %d, %d, %d, read, random, %fms, %fMB/s\n", arg_p->nb_threads, arg_p->block_size, arg_p->n_lrepeat, time, finalresult);
				output(arg_p->fmutex_p,arg_p->results_fp,output_s);
			}
			if(arg_p->write){
				fp = fopen(tmp_file_name, "w+") ;
				random_writing(&result, arg_p->block_size, fp, arg_p->n_lrepeat);
				time = (float)result/(CLOCKS_PER_SEC/1000)/arg_p->n_lrepeat;
				finalresult = clockstoMBpS(arg_p->block_size, result, arg_p->n_lrepeat);
				sprintf(output_s, "disk, %d, %d, %d, write, random, %fms, %fMB/s\n", arg_p->nb_threads, arg_p->block_size, arg_p->n_lrepeat, time, finalresult);
				output(arg_p->fmutex_p,arg_p->results_fp,output_s);
			}
		}
	}
	int status = remove(tmp_file_name);
	//	if( status == 0 )
	//		printf("%s file deleted successfully.\n",tmp_file_name);
	//	else
	//		printf("Unable to delete the file %s\n",tmp_file_name);
	free(tmp_file_name);
	free(output_s);
	return(NULL);
}

int print_usage(char* argv[]) {
	fprintf(stderr, "Usage:\t%s [-l][-w|-r] [-a|-s] [-b B] [-h] [-o O] [-n N] [-t T]\n", argv[0]);
	fprintf(stderr, "\t-a\tFlag for random benchmarking\n");
	fprintf(stderr, "\t-b B\tBlock size [default: 1kB]\n");
	fprintf(stderr, "\t-h\tShow this help screen\n");
	fprintf(stderr, "\t-l\tFlag for latency benchmarking\n");
	fprintf(stderr, "\t-o O\tNumber of operations per loop [default: 1E9]\n");
	fprintf(stderr, "\t-n N\tNumber of time the benchmark is repeated [default: 1]\n");
	fprintf(stderr, "\t-r\tFlag for read benchmarking\n");
	fprintf(stderr, "\t-s\tFlag for sequential benchmarking\n");
	fprintf(stderr, "\t-t T\tNumber of threads to run the benchmark on [default: 1]\n");
	fprintf(stderr, "\t-w\tFlag for write benchmarking\n");
	fprintf(stderr, "Using multiple flags benchmark will benchmark all possible combos\n"
			"ex : \"-w -r -s -a\" will compute sequential r&w and random r&w\n");
	return 0;
}

// Initiates & runs the disk benchmark.
int main(int argc, char** argv){
	srand(time(NULL));
	char c;
	FILE * results_fp = fopen("disk_benchmark_results.txt", "a+");
	int arg_selected = 0; // = 0 => no arg selected => use default args
	int n_threads = 1;
	pthread_mutex_t fmutex = PTHREAD_MUTEX_INITIALIZER;   // mutex protecting the final result file
	bmrk_arg arg;
	arg.block_size = 1024;
	arg.fmutex_p = &fmutex;
	arg.latency = 0;
	arg.sequential = 0;
	arg.n_lrepeat = 100;    // loop repetion. ex : write 100 blocks of 1kB
	arg.n_brepeat = 1;      // nb of repetition of total benchmark
	arg.random = 0;
	arg.read = 0;
	arg.results_fp = results_fp;
	arg.write = 0;

	// Parsing the command line
	while ((c = getopt(argc, argv, "ab:hln:o:rst:w")) != -1)
		switch (c) {
		case'a': // random
			arg.random = 1;
			arg_selected = 1;
			break;

		case 'b': // block size
			arg.block_size = atoi(optarg);
			break;

		case 'h': // help
			print_usage(argv);
			exit(0);

		case 'l': // latency
			arg.latency = 1;
			arg_selected = 1;
			break;

		case 'o': // number of type we repeat the benchmark
			arg.n_lrepeat = atoi(optarg);
			break;

		case 'n': // number of type we repeat the benchmark
			arg.n_brepeat = atoi(optarg);
			break;

		case 'r': // read
			arg.read = 1;
			arg_selected = 1;
			break;

		case 's': // sequential
			arg.sequential = 1;
			arg_selected = 1;
			break;

		case 't': // number of threads
			n_threads = atoi(optarg);
			arg.nb_threads = n_threads;
			break;

		case 'w': // write
			arg.write = 1;
			arg_selected = 1;
			break;

		case '?': // help
			print_usage(argv);
			return 1;

		default:
			abort ();
		}
	if(arg_selected == 0){
		printf("Disk benchmark - default\n");
		arg.write = 1;
		arg.sequential = 1;
	}
	else if(arg.latency == 0 && arg.random + arg.sequential == 0 && arg.read + arg.write == 0){
		printf("Disk benchmark error - non sufficient arguments\n");
		exit(0);
	}


	switch(n_threads){
	case 1:
		bmrk_launcher((void*) &arg);
		break;
	default:
		if(n_threads > MAX_THREAD){
			printf("too many threads\n");
			exit(0);
		}
		else{
			int i;
			pthread_t threads[n_threads];
			for(i = 0; i < n_threads; i++){
				pthread_create(&(threads[i]), NULL, bmrk_launcher, (void*) &arg);
			}
			for(i = 0; i < n_threads; i++){
				pthread_join(threads[i],NULL);
			}
		}
		break;
	}
	fclose(results_fp);

	return(0);
}

