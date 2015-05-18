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

int BYTES = 1024;
int NB_THREADS = 1;
int PROC = 0;
long N_OPERATIONS = 1000000;
int LATENCY=0;

int print_usage(char* argv[]) {
	fprintf(stderr, "Usage:\t%s [-p|0/1] [-h] [-b B] [-o O] [-t T]\n", argv[0]);
	fprintf(stderr, "\t-p\tFlag for Protocol to be used TCP=0 & UDP=1\n");
	fprintf(stderr, "\t-h\tShow this help screen\n");
	fprintf(stderr,"\t-o 0\tNumber of operation per loop: \n");
	fprintf(stderr,"\t-b B\tNumber of bytes to send through network \n");
	fprintf(stderr,"\t-t T\tNumber of threads to run the benchmark on \n");
	fprintf(stderr,"\t-l L\tFlag for latency or throughput LTC=1 & THRPT=0 \n");
	return 0;
}

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char * client_message[BYTES];
    //Receive a message from client
    while( (read_size = recv(sock , client_message , BYTES , 0)) > 0 )
    {
    	//printf("server TCP received packet %d of size %d\n", i++, read_size);
    }
    if(read_size == 0)
    {
        //puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    //Free the socket pointer
    free(socket_desc);

    return 0;
}

void* ServerTCP(void *arg) {

	/* ------------  SERVER --------------*/
	struct sockaddr_in saddr_in;
	int* new_sock;
	int sock_serv,c,sock,ret;

// --------------  SOCKET CREATION  ------------- //
	sock_serv = socket(PF_INET, SOCK_STREAM, 0);
	if (sock_serv == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	memset(&saddr_in, 0, sizeof(saddr_in));
	saddr_in.sin_family = AF_INET;
	saddr_in.sin_port = htons(10008);
	saddr_in.sin_addr.s_addr = htonl(INADDR_ANY );
// BIND //
	if (bind(sock_serv, (struct sockaddr *) &saddr_in, sizeof(saddr_in))
			== -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	if
// LISTEN //
	(listen(sock_serv, SOMAXCONN) == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	int length = sizeof(saddr_in);
// ACCEPT //
	c = sizeof(struct sockaddr_in);
	    while( (sock = accept(sock_serv, (struct sockaddr *)&saddr_in, (socklen_t*)&length)) )
	    {
	       // puts("Connection accepted");

	        pthread_t sniffer_thread;
	        new_sock = malloc(1);
	        *new_sock = sock;

	        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
	        {
	            perror("could not create thread");
	            return NULL;
	        }

	        //Now join the thread , so that we dont terminate before the thread
	        pthread_join( sniffer_thread , NULL);
	       // puts("Handler assigned");
	    }

	    if (sock < 0)
	    {
	        perror("accept failed");
	        return NULL;
	    }

	close(sock_serv);
	pthread_exit(&ret);
}

void* ServerUDP(void *arg) {

	struct sockaddr_in si_me, si_other;
	int s, i;
	socklen_t slen = sizeof(si_other);
	char* buf = malloc(BYTES);
	ssize_t byte_received = 0;

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		printf("socket");

	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(10009);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY );
	if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
		printf("bind");

	for (i=0; i<N_OPERATIONS*NB_THREADS; i++) {
	         if ((byte_received = recvfrom(s, buf, BYTES, 0, (struct sockaddr *)&si_other, &slen)) == -1){
	            printf("recvfrom()");
	         } else {
	        	 //printf("Received packet of size %d\n", byte_received);
	        	 //printf("Received packet from %s:%d\nData: %s\n\n",inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), buf);
	         }
	        }

	close(s);
	return NULL;
}

void* ClientTCP(void *arg) {



	struct sockaddr_in sock_host;
	int sock,ret;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	memset(&sock_host, '\0', sizeof(sock_host));
	sock_host.sin_family = AF_INET;
	sock_host.sin_port = htons(10008);
	inet_aton("127.0.0.1", &sock_host.sin_addr);

	if (connect(sock, (struct sockaddr *) &sock_host, sizeof(sock_host)) < 0) {
		perror("connect()");
		exit(errno);
	}



	char* buffer;
	buffer = (char*) malloc(BYTES);
	if (LATENCY == 0){
	memset(buffer, 1, BYTES);
	}
	int len = strlen(buffer);

	int j = 0;
	ssize_t bytes_sent = 0;
	while (j < N_OPERATIONS) {

		if ((bytes_sent = send(sock, buffer, len, 0)) < 0) {
			perror("send()");
			exit(errno);
		} else {
			//printf("tcp client sent %d bytes\n", bytes_sent);
		}
		j++;
	}
	close(sock);
	pthread_exit(&ret);
}

void* ClientUDP(void *arg) {

	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other);
	int i=0;
	char* buffer = malloc(BYTES);
	if(LATENCY ==0){
	memset(buffer, 1, BYTES);
	}
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		printf("socket");

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(10009);
	if (inet_aton("127.0.0.1", &si_other.sin_addr) == 0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
if (BYTES > 65530){
	for (i = 0; i < N_OPERATIONS*2; i++) {

			if ((sendto(s, buffer, BYTES/2, 0, (struct sockaddr *)&si_other, slen) == -1)) {
				perror("sendto()");
				exit(errno);
			}
		}
}
else{
	for (i = 0; i < N_OPERATIONS; i++) {

		if (sendto(s, buffer, BYTES, 0, (struct sockaddr *)&si_other, slen) == -1) {
			perror("sendto()");
			exit(errno);
		}
	}
}
	close(s);
	return NULL;
}

int main(int argc, char **argv) {
	pthread_t tid[NB_THREADS + 1];
	int err1;
	int c;


	while ((c = getopt(argc, argv, "hb:p:o:t:l:")) != -1)
		switch (c) {
		case 'p':
			PROC = atoi(optarg);
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

	struct timeval tim;
	gettimeofday(&tim, NULL );
	double t1 = tim.tv_sec + (tim.tv_usec / 1000000.0);

	if (PROC == 0) { // 0 => TCP
		err1 = pthread_create(&(tid[0]), NULL, &ServerTCP, NULL );
		if (err1 != 0)
			printf("\ncan't create thread :[%s]", strerror(err1));


		sleep(1);
		int err[NB_THREADS];


		int k = 0;
		for (k = 1; k < NB_THREADS + 1; k++) {
			err[k] = pthread_create(&(tid[k]), NULL, &ClientTCP, NULL );
			if (err[k] != 0)
				printf("\ncan't create thread :[%s]", strerror(err[k]));

		}

		k=0;
				for (k = 1; k < NB_THREADS+1; k++) {

					if(pthread_join(tid[k], NULL )!=0)

						printf("join error()");

				}



		gettimeofday(&tim, NULL );
		double t2 = tim.tv_sec + (tim.tv_usec / 1000000.0);


		double final_time = t2 - t1 - 1;
		double final_t = (double) final_time;
		double numerator = (double) (BYTES*N_OPERATIONS*NB_THREADS);
		double throughput= numerator/final_t;

		if (LATENCY == 1){
			printf("network,TCP,%d,%d,%d,latency,%.6lf s\n", N_OPERATIONS,NB_THREADS,BYTES,final_time);
		}
		else if (LATENCY == 0){
			printf("network,TCP,%d,%d,%d,throughput,%.6lf MBps\n",N_OPERATIONS,NB_THREADS,BYTES,throughput/1000000);
		}
	//pthread_join(tid[0], NULL );
		//sleep(5);



	} else if (PROC == 1) { //1 => UDP

		err1 = pthread_create(&(tid[0]), NULL, &ServerUDP, NULL );
		if (err1 != 0)
			printf("\ncan't create thread :[%s]", strerror(err1));


		sleep(1);

		int err[NB_THREADS];
		int j;
		for (j = 1; j < NB_THREADS+1; j++) {

			err[j] = pthread_create(&(tid[j]), NULL, &ClientUDP, NULL );

			if (err[j] != 0)
				printf("\ncan't create thread :[%s]", strerror(err[j]));


		}

		int k=0;
		for (k = 1; k < NB_THREADS+1; k++) {

			if(pthread_join(tid[k], NULL )!=0)

				printf("join error()");

		}



		gettimeofday(&tim, NULL );
		double t2 = tim.tv_sec + (tim.tv_usec / 1000000.0);

		double final_time = t2 - t1 - 1;
		double final_t = (double) final_time;
		double numerator = (double) (BYTES*N_OPERATIONS*NB_THREADS);
		double throughput= numerator/final_t;

		if (LATENCY == 1){
				printf("network,UDP,%d,%d,%d,latency,%.6lf s\n", N_OPERATIONS,NB_THREADS,BYTES,final_time);
				}
				else if (LATENCY == 0){
					printf("network,UDP,%d,%d,%d,throughput,%.6lf MBps\n",N_OPERATIONS,NB_THREADS,BYTES,throughput/1000000);
				}
		//pthread_join(tid[0], NULL );
		//sleep(5);

	}

	return 0;
}



