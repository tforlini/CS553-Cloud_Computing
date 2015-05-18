/*
 z * memory.c
 *
 *  Created on: Sep 22, 2014
 *      Author: tforlini
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <memory.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int ACCESS = 0;
int BYTES = 1024;
int N_OPERATIONS = 1000;
int NB_THREADS = 1;
int LATENCY = 1;


int print_usage(char* argv[]) {
	fprintf(stderr, "Usage:\t%s [-a|0/1] [-h] [-b B] [-o O] [-t T] [-l L]\n", argv[0]);
	fprintf(stderr,"\t-a\tFlag for Sequential or random access to memory SEQ=0 & RAND=1\n");
	fprintf(stderr, "\t-h\tShow this help screen\n");
	fprintf(stderr, "\t-o 0\tNumber of operation per loop: \n");
	fprintf(stderr, "\t-b B\tNumber of bytes in block \n");
	fprintf(stderr, "\t-t T\tNumber of threads to run the benchmark on \n");
	fprintf(stderr,"\t-l L\tFlag for latency or throughput LTC=1 & THRPT=0 \n");
	return 0;
}

void *WriteSequential(void *arg) {
	char* mem = (char*)arg;
	int i, ret;
	for (i = 0; i < N_OPERATIONS; ++i) {
		memset(&mem[i*BYTES], '1', BYTES);
	}

	pthread_exit(&ret);
}

void *ReadSequential(void *arg) {
	char* mem = (char*)arg;
	char temp;
	int i, ret;

	for (i = 0; i < N_OPERATIONS; ++i) {
		temp = *strchr(&mem[i*BYTES], '1');
	}
	pthread_exit(&ret);
}

void* WriteRandom(void *arg) {
	char* mem = (char*)arg;
	int i, ret;
	char temp = '0';
	int idx;

	for (i = 0; i < N_OPERATIONS; ++i) {
		idx = rand() % N_OPERATIONS;
		memset(&mem[idx*BYTES], temp, BYTES);
	}
	pthread_exit(&ret);
}

void* ReadRandom(void *arg) {
	char* mem = (char*)arg;
	int i, ret;
	char temp;
	int idx;

	for (i = 0; i < N_OPERATIONS; ++i) {
		idx = rand() % N_OPERATIONS;
		temp = *strchr(&mem[idx*BYTES], '1');
	}
	pthread_exit(&ret);
}

int main(int argc, char **argv) {

int c;

	while ((c = getopt(argc, argv, "hb:a:o:t:l:")) != -1)
		switch (c) {
		case 'a':
			ACCESS = atoi(optarg);
			break;
		case 'h':
			print_usage(argv);
			exit(0);
		case 'b':
			BYTES = atoi(optarg);
			break;
		case 'o':
			N_OPERATIONS = (long) (atof(optarg));
			break;
		case 't':
			NB_THREADS = atoi(optarg);
			break;
		case '?':
			print_usage(argv);
			return 1;
		case 'l':
			LATENCY = atoi(optarg);
			break;
		default:
			abort();
		}

	int k,j,l;
	pthread_t tid[NB_THREADS*2];
	int err_write[NB_THREADS];
	int err_read[NB_THREADS];
	void* mem_char = malloc(BYTES*N_OPERATIONS);
	struct timeval tim;
	gettimeofday(&tim, NULL );
	double t1 = tim.tv_sec + (tim.tv_usec / 1000000.0);

	if (ACCESS == 0) {

		for (k = 0; k < NB_THREADS ; k++) {

			err_write[k] = pthread_create(&(tid[k]), NULL, WriteSequential, mem_char );
			if (err_write[k] != 0)
				printf("\ncan't create thread :[%s]", strerror(err_write[k]));

		}
		for (j = 0; j < NB_THREADS; j++) {

			err_read[j] = pthread_create(&(tid[j+NB_THREADS]), NULL, ReadSequential, mem_char );
			if (err_read[j] != 0)
				printf("\ncan't create thread :[%s]", strerror(err_read[j]));

		}

void* end;

		for (l = 0; l < NB_THREADS*2; l++) {

			if (pthread_join(tid[l], &end ) != 0){
				printf("join error()");
			}
		}

		gettimeofday(&tim, NULL );
				double t2 = tim.tv_sec + (tim.tv_usec / 1000000.0);
				double final_time = (t2 - t1);
				double numerator = (double) (BYTES*N_OPERATIONS*NB_THREADS);
				double throughput= numerator/final_time;

				if (LATENCY == 1){
					printf("memory,sequential,%d,%d,%d,latency,%.6lf\n", N_OPERATIONS,NB_THREADS,BYTES,final_time);
				}
				else if (LATENCY == 0){
					printf("memory,sequential,%d,%d,%d,throughput,%.6lf\n",N_OPERATIONS,NB_THREADS,BYTES,throughput/1000000);
				}

	} else if (ACCESS == 1) {


				for (k = 0; k < NB_THREADS ; k++) {

					err_write[k] = pthread_create(&(tid[k]), NULL, WriteRandom, mem_char );
					if (err_write[k] != 0)
						printf("\ncan't create thread :[%s]", strerror(err_write[k]));

				}
				for (j = 0; j < NB_THREADS; j++) {

					err_read[j] = pthread_create(&(tid[j+NB_THREADS]), NULL, ReadRandom, mem_char );
					if (err_read[j] != 0)
						printf("\ncan't create thread :[%s]", strerror(err_read[j]));

				}

		void* end;

				for (l = 0; l < NB_THREADS*2; l++) {

					if (pthread_join(tid[l], &end ) != 0){
						printf("join error()");
					}
				}


				gettimeofday(&tim, NULL );
						double t2 = tim.tv_sec + (tim.tv_usec / 1000000.0);
						double final_time = (t2 - t1);
						double numerator = (double) (BYTES*N_OPERATIONS*NB_THREADS);
						double throughput= numerator/final_time;

						if (LATENCY == 1){
							printf("memory,random,%d,%d,%d,latency,%.6lf\n", N_OPERATIONS,NB_THREADS,BYTES,final_time);
						}
						else if (LATENCY == 0){
							printf("memory,random,%d,%d,%d,throughput,%.6lf\n",N_OPERATIONS,NB_THREADS,BYTES,throughput/1000000);
						}
	}
	return 0;
}

