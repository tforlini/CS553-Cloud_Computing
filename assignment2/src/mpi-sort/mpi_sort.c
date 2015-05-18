/*
 * mpi_sort.c
 *
 *  Created on: Oct 17, 2014
 *      Author: Thomas Dubucq
 *  Using http://www.lam-mpi.org/tutorials/one-step/ezstart.php as a base.
 */


#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include "int_list.h"
#include "sort.h"

/* GENERAL SETTINGS */
#define FILENAME_SIZE 64
#define MAX_MERGE_SLAVE_POS 1
#define FILENAME_POS 2
#define shard_size_POS 3

#define DIETAG -1
#define IDLETAG 0
#define SORTTAG 1
#define MERGETAG 2
#define SENDTAG	3

/* Local functions (used in main) */
static void master(int max_merge_slaves, char* filename, int shard_size);
static void slave(int shard_size);

/* Local types */
typedef enum {IDLE, SORT, MERGE, SEND} slave_state_t;
typedef enum {SORTING, SORT_MERGING, MERGING} master_state_t;
typedef struct {slave_state_t state; int numb_shard;} slave_status_t;

int main(int argc, char **argv){
	int shard_size, myrank, max_merge_slaves;
//	printf("Main : start\n\n");
	/* Arguments parsing */
	char* filename = (char*)malloc(FILENAME_SIZE*sizeof(char));
	sscanf(argv[shard_size_POS], "%d", &shard_size);
//	char* tmp_string = (char*)malloc(20*sizeof(char));
//	sprintf(tmp_string, "\%%ds", FILENAME_SIZE);
	sscanf(argv[FILENAME_POS], "%s",filename);
//	free(tmp_string);
	sscanf(argv[MAX_MERGE_SLAVE_POS], "%d", &max_merge_slaves);

	/* Initialize MPI */
	MPI_Init(&argc, &argv);

	/* Find out my identity in the default communicator */
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	if (myrank == 0) {
		master(max_merge_slaves, filename, shard_size);
	} else {
		slave(shard_size);
	}

	/* Shut down MPI */
	MPI_Finalize();
	return 0;

}

/* Extract a shard from a file */
static int extract_shard(FILE* file, int* buffer, int shard_size){
	int i = 0;
	int tmp;
	while(fscanf(file, "%d", &tmp) != EOF && i < shard_size){
		buffer[i] = tmp;
		i++;
	}

	if(i == shard_size)
		return(1);
	else
		return(0);

}

/* Send a sort message to a slave*/
static void send_sort(int* shard, int shard_size, int rank){
	printf("send_sort\n");
	MPI_Send(shard,             	/* message buffer */
			shard_size,             /* one data item */
			MPI_INT,       			/* data item is a single datashard */
			rank,              		/* destination process rank */
			SORTTAG,           		/* sort tag */
			MPI_COMM_WORLD);  		/* default communicator */

}

/* Ask a slave to send its data to another*/
static void send_send(int slave_sender_rank, int slave_receiver_rank){

	/* Warn receiver */
	printf("sendsend sender_rank %d,recv rank%d\n",slave_sender_rank, slave_receiver_rank);
	MPI_Send(&slave_sender_rank,        /* message buffer */
				1,                      /* one data item */
				MPI_INT,      		 	/* data item is an int */
				slave_receiver_rank,    /* destination process rank */
				MERGETAG,           	/* merge tag */
				MPI_COMM_WORLD);  		/* default communicator */

	/* Ask sending */
	MPI_Send(&slave_receiver_rank,             	/* message buffer */
			1,                      /* one data item */
			MPI_INT,      		 	/* data item is an int */
			slave_sender_rank,      /* destination process rank */
			SENDTAG,           		/* send tag */
			MPI_COMM_WORLD);  		/* default communicator */
}

void print_status(master_state_t state, int nb_slaves, slave_status_t* status){
	int i;
	printf("\n --- Master status --- \n");
	switch (state) {
		case SORTING:
			printf("sorting\n");
			break;
		case SORT_MERGING:
			printf("sorting & merging\n");
			break;
		case MERGING:
			printf("merging\n");
			break;
	}
	printf(" --- Slave  status --- \n");
	char sort[5] = "SORT";
	char merge[6] = "MERGE";
	char send[5] = "SEND";
	char idle[5] = "IDLE";
	for(i = 0; i < nb_slaves; i++){
		switch(status[i].state){
		case IDLE:
			printf("%d, %d shards, %s\n",i, status[i].numb_shard, idle);
			break;
		case SORT:
			printf("%d, %d shards, %s\n",i, status[i].numb_shard, sort);
			break;
		case MERGE:
			printf("%d, %d shards, %s\n",i, status[i].numb_shard, merge);
			break;
		case SEND:
			printf("%d, %d shards, %s\n",i, status[i].numb_shard, send);
			break;
		}
	}
	printf(" --------------------- \n");
}
/* Master */
/* --- Behavior ---
 * Master starts to assign a sort job to each slave.
 *
 * Then it consists in a 3-states machine state.
 * 		state SORT
 * 			assigning slaves to merging jobs until max_merge_slaves is reached
 * 		state SORT_MERGE
 * 			parallel merging and sorting. Stays in this state until shards_left = 0;
 * 		state MERGE
 * 			all shards have been assigned. Gathering/merging of sorted data. stops when there is all merging slaves are set to idle(also exits the state-machine).
 * */
static void master(int max_merge_slaves, char* filename, int shard_size){
	int ntasks, rank, total_num_shards, shard_left, first_round_rank, i, result;
	first_round_rank = 0; // will give the rank of a slave when he will finish the 1st round of sort
	int* shard = (int*)malloc(sizeof(int)*shard_size);
	MPI_Status* status_p = (MPI_Status*)malloc(sizeof(MPI_Status));
	MPI_Status status = *status_p;

	/* Find out how many processes there are in the default communicator */
	MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
//	printf("ntask =%d\n",ntasks);

	/* Status of workers */
	slave_status_t slave_status[ntasks]; 	// store the current status of a slave and the number of shards its possesses
	int mergers[max_merge_slaves];			// register the ranks of slaves assigned to merging
	int_list_t onhold_sorters = NULL;		// register the ranks of slaves that are waiting to send their sorted data
	int final_merge = 0;					// flag put to 1 when the merger have all the data and have to gather it onto a single merger
	int onhold_merger = 0;					// will register if a merger is idle during final merge
	/* Open the input data file */
	FILE * data_file = fopen(filename, "r");
	free(filename);

	/* Seed the slaves; 1srt round of sort */
	for (rank = 1; rank < ntasks; ++rank) {
		/* Update slave status */
		slave_status[rank].numb_shard =  1;
		slave_status[rank].state = SORT;

		if(extract_shard(data_file,shard,shard_size) == 0){
			free(shard);
			printf("Error : Master- not enough shards for all workers on first round\n");
			/* Tell all the slaves to exit by sending an empty message with the DIETAG */
			for (rank = 1; rank < ntasks; ++rank) {

				MPI_Send(0, 0, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);
			}
			exit(0);
		}else{
			/* Send a shard to each rank */
			send_sort(shard, shard_size, rank);
			total_num_shards++;
		}
	}


	/* Loop over getting new work requests until there is no more work to be done */
	int sorted = max_merge_slaves - 1; 	// will be decreased each time a merger is put back to idle
											// the mail while loop will then stop when there is only one merger left
										// in SORT_MERGE state : next merger to send data if none are idle.
	int slave_merge_numb = 0;			// in SORT state : gives the number of slaves assigned to merging
	master_state_t state = SORTING;

	while (sorted != 0) {
		print_status(state, ntasks, slave_status);
		/* Receive message from a slave */
//		printf("Master receive1\n");
		MPI_Probe(MPI_ANY_SOURCE, IDLETAG, MPI_COMM_WORLD, &status);
//		printf("status size = %d, sizeof(int) = %d\n", (int)status._ucount, (int)sizeof(int));
		MPI_Recv(&result,           /* message buffer */
				1,                 	/* one data item */
				MPI_INT,        	/* of type integer */
				MPI_ANY_SOURCE,   	/* receive from any sender */
				IDLETAG,       		/* IDLE type of message */
				MPI_COMM_WORLD,    	/* default communicator */
				&status);          	/* info about the received message */
		switch(state){
		case SORTING: // transition between no mergers to max_merge_slaves mergers
		{
			switch(result){
			case 0: // slave has sent its data
				if(shard_left != 0){
					if(extract_shard(data_file,shard,shard_size) == 0){
						shard_left = 0;
						free(shard);
						slave_status[status.MPI_SOURCE].numb_shard = 0;
						slave_status[status.MPI_SOURCE].state = IDLE;

					}else{
						/* Send a shard */
						slave_status[status.MPI_SOURCE].numb_shard = 1;
						slave_status[status.MPI_SOURCE].state = SORT;
						send_sort(shard, shard_size, rank);
						total_num_shards++;
					}
				}else{// no shard left -> slave put to idle
					slave_status[status.MPI_SOURCE].numb_shard = 0;
					slave_status[status.MPI_SOURCE].state = IDLE;
				}
				break;
			case 1: // slave has sorted its data -> send it to merger
					// 1 on 2 slaves is assigned to merging until max_merge_slaves is reached
				if(first_round_rank % 2 == 0){ // set to merger
					/* Register slave as a merger*/
					mergers[slave_merge_numb] = status.MPI_SOURCE;
					slave_status[status.MPI_SOURCE].state = IDLE;
					slave_merge_numb++;

					if(slave_merge_numb == max_merge_slaves){ // MAX_MERGE_SLAVE reached -> state change
						if(shard_left ==0){
							state = MERGING;
						}else{
							state = SORT_MERGING;
						}

					}
				}else{// ask slave to send its shard to last merger registered
//					printf("Master : send-send source%d merger%d\n",status.MPI_SOURCE,mergers[slave_merge_numb-1]);
					send_send(status.MPI_SOURCE,mergers[slave_merge_numb-1]);
					slave_status[status.MPI_SOURCE].state = SEND;
					slave_status[mergers[slave_merge_numb-1]].state = MERGE;
				}
				first_round_rank++;
				break;
			default: //merger has merged its data
				slave_status[status.MPI_SOURCE].numb_shard = result;
				slave_status[status.MPI_SOURCE].state = IDLE;
				break;
			}
		}

		break;
		case SORT_MERGING: // state until shard_left = 0
		{
			switch(result){
			case 0: // slave has sent its data -> give it new shard
				if(shard_left != 0){
					if(extract_shard(data_file,shard,shard_size) == 0){
						shard_left = 0;
						free(shard);
						slave_status[status.MPI_SOURCE].numb_shard = 0;
						slave_status[status.MPI_SOURCE].state = IDLE;
						state = MERGING;

					}else{
						/* Send a shard */
						slave_status[status.MPI_SOURCE].numb_shard = 1;
						slave_status[status.MPI_SOURCE].state = SORT;
						send_sort(shard, shard_size, rank);
						total_num_shards++;
					}
				}else{// no shard left -> slave put to idle
					slave_status[status.MPI_SOURCE].numb_shard = 0;
					slave_status[status.MPI_SOURCE].state = IDLE;
					state = MERGING;
				}
				break;

			case 1: // slave has sorted its data -> send its data to a merger
				/* Find an idle merger */
				i = 0;
				int merger_found = 0;
				while(merger_found == 0 && i < max_merge_slaves){
					if(slave_status[mergers[i]].state == IDLE)
						merger_found = mergers[i];
					i++;
				}
				/* If no merger in IDLE state*/
				if(merger_found == 0){
					add(status.MPI_SOURCE, onhold_sorters);
				}else{
					send_send(status.MPI_SOURCE,mergers[merger_found]);
					slave_status[status.MPI_SOURCE].state = SEND;
					slave_status[mergers[slave_merge_numb-1]].state = MERGE; // send merge request to last merger registered
				}
				break;

			default: //merger has merged its data
				slave_status[status.MPI_SOURCE].numb_shard = result;

				/* Check onhold_list */
				if(onhold_sorters != NULL){
					int sender = hd(onhold_sorters);
					onhold_sorters = free_list_el(onhold_sorters);

					send_send(sender,status.MPI_SOURCE);
					slave_status[sender].state = SEND;
				}else{
					slave_status[status.MPI_SOURCE].state = IDLE;
				}
				break;
			}
		}
			break;

		case MERGING: // last state : processing last shards, and merging all the sorted data.
		{
			switch(result){
			case 0: // slave has sent its data,no shard left -> put it to idle
				slave_status[status.MPI_SOURCE].numb_shard = 0;
				slave_status[status.MPI_SOURCE].state = IDLE;
				break;

			case 1: // slave has sorted its data -> send its data to a merger
				/* Find an idle merger */
				i = 0;
				int merger_found = 0;
				while(merger_found == 0 && i < max_merge_slaves){
					if(slave_status[mergers[i]].state == IDLE)
						merger_found = mergers[i];
					i++;
				}
				if(merger_found == 0){
					merger_found = slave_merge_numb;
					slave_merge_numb = (slave_merge_numb + 1)% max_merge_slaves;
				}
				send_send(status.MPI_SOURCE,mergers[merger_found]);
				break;

			default: // merger has merged its data
				slave_status[status.MPI_SOURCE].numb_shard = result;
				if(final_merge == 0){
					/* Check onhold_list */
					if(onhold_sorters != NULL){
						int sender = hd(onhold_sorters);
						onhold_sorters = free_list_el(onhold_sorters);

						send_send(sender,status.MPI_SOURCE);
						slave_status[sender].state = SEND;
					}else{
						/*Check if there is slaves still sorting*/
						i = 0;
						int sorter_found = 0;
						while(sorter_found == 0 && i < ntasks){
							if(slave_status[i].state == SORT)
								sorter_found = 1;
							i++;
						}

						if(sorter_found == 0){
							final_merge = 1;
						}else{
							slave_status[status.MPI_SOURCE].state = IDLE;
						}
					}
				}else{ // case final_merge = 1
					if(onhold_merger == 0){ // no merger on hold
						onhold_merger = status.MPI_SOURCE;
					}else{
						/* Find which merger has the most data and send him the reamining data */
						if(result > slave_status[onhold_merger].numb_shard){
							send_send(onhold_merger, status.MPI_SOURCE);
							slave_status[onhold_merger].state = SEND;
							for(i = 0; i < max_merge_slaves; i++){
								if(mergers[i] == onhold_merger){
									mergers[i] = -1;
								}
							}
						}else{
							send_send(status.MPI_SOURCE, onhold_merger);
							slave_status[status.MPI_SOURCE].state = SEND;
							for(i = 0; i < max_merge_slaves; i++){
								if(mergers[i] == status.MPI_SOURCE){
									mergers[i] = -1;
								}
							}
						}
						sorted--; //one less merger slave is active
					}
				}//endif final_merge == 0
				break;
			}
		}
			break;
		}

	} // end while sorted != 0

	/* There's no more work to be done, so receive all the outstanding results from the slaves */
	int last_merger = 0;
	for(i = 0; i < max_merge_slaves; i ++){
		if(mergers[i] != -1){
			if(last_merger != 0){
				printf("Error: Master- more than 1 last merger\n");
				exit(0);
			}else
				last_merger = mergers[i];
		}
	}

	int total = slave_status[last_merger].numb_shard* shard_size;
	int* data = (int*)malloc(total*sizeof(int));
	/* Retrieve data */
	send_send(last_merger, 0);
	MPI_Recv(data, total, MPI_INT, 0, SORTTAG, MPI_COMM_WORLD, &status);

	/* Tell all the slaves to exit by sending an empty message with the DIETAG */
	for (rank = 1; rank < ntasks; ++rank) {
		MPI_Send(0, 0, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);
	}

	/* writing results */
	printf("Master : Slaves put to sleep\n  writing data\n");

	FILE *f = fopen("results.txt", "w");
	if (f == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	for(i = 0; i < total; i++)
		fprintf(f, "%d\n", data[i]);

	fclose(f);


}

/* Send a "done" message to master */
static void send_done(int numb_shards){
	int mess = numb_shards;
//	printf("send_done\n");
	MPI_Send(&mess, 1, MPI_INT, 0, IDLETAG, MPI_COMM_WORLD);
}

/* Slave */
static void slave(int shard_size){
	int sender,size_data2, receiver;
	int* data1;
	int size_data1 = 0;
	MPI_Status status;
	slave_state_t state = IDLE;
	status.MPI_TAG =0;
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	while (1) {
		switch(state){
		case IDLE:
		{
			/* Receive a message from the master */
//			printf("Slave%d IDLE\n",myrank);
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//			printf("Slave%d probe\n",myrank);
//			printf("status size = %d, sizeof(int) = %d, tag = %d\n", (int)status._ucount, (int)sizeof(int), status.MPI_TAG);
//			fflush(stdout);
//			MPI_Recv(&numb_packet, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
//			printf("Slave%d receive1 received correctly\n",myrank);
			/* Check the tag of the received message */
			switch(status.MPI_TAG){
			case DIETAG:
				return;
				break;
			case IDLETAG:
				state = IDLE;
				break;
			case SORTTAG:
				state = SORT;
				break;
			case MERGETAG:
				state = MERGE;
				break;
			case SENDTAG:
				state = SEND;
				break;
			default:
				printf("Message tag unknown\n");
				state = IDLE;
				break;
			}
			break;
		}
		case SORT:
//			printf("Slave%d SORT\n",myrank);
			/* Receive a message from the master with the shard to sort */
			data1 = (int*)malloc(shard_size* sizeof(int));
			size_data1 = shard_size*sizeof(int);
//			printf("Slave receive2\n");
			MPI_Recv(data1, shard_size, MPI_INT, 0, SORTTAG, MPI_COMM_WORLD, &status);

			/* Sorting */
			int_qsort(data1, 1);

			/* Inform master */
//			printf("Slave%d sortdone\n",myrank);
			send_done(1);
			state = IDLE;
			break;
		case MERGE:
//			printf("Slave%d MERGE\n",myrank);
			MPI_Recv(&sender, 1, MPI_INT, 0, MERGETAG, MPI_COMM_WORLD, &status);
//			printf("Slave%d MERGE sender=%d\n",myrank,sender);
			/* Receive a message from another slave with the shards to merge
			 * the previous message gives the number of shards to expect */
			MPI_Probe(sender, SENDTAG, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_INT, &size_data2);
			int* data2 = (int*)malloc(size_data2*sizeof(int));
//			printf("Slave%d probe: source = %d, tag =%d, _count=%d, datasize2=%d\n",myrank,status.MPI_SOURCE,status.MPI_TAG,(int)status._ucount,size_data2);
			MPI_Recv(data2, size_data2, MPI_INT,
					MPI_ANY_SOURCE, SENDTAG, MPI_COMM_WORLD, &status);
//			printf("Slave%d MERGE recv done\n",myrank);
//			fflush(stdout);
			/* Merging in data1*/
			data1 = merge(data1, size_data1, data2, size_data2);
			size_data1 = size_data1 + size_data2;
			free(data2);

			/* Inform master */
			send_done(size_data1/shard_size);
			state = IDLE;
			break;

		case SEND:
			/* Get receiver id */
//			printf("Slave%d SEND\n",myrank);
			MPI_Recv(&receiver, 1, MPI_INT, 0, SENDTAG, MPI_COMM_WORLD, &status);
			/* Send accumulated data */
//			printf("Slave%d SEND info: datasize2\1=%d\n",myrank,size_data1);
//			printf("Slave%d SEND info: datasize1=%d sizeof=%d\n",myrank, size_data1, (int)sizeof(data1));
			MPI_Send(data1, size_data1, MPI_INT, receiver, SENDTAG, MPI_COMM_WORLD);
			free(data1);
			size_data1 = 0;

			/* Inform master */
			send_done(0);
			state = IDLE;
			break;
		}

	}
}


