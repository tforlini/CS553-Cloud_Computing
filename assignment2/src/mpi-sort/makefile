OBJS = int_list.o sort.o
CC = mpicc

FLAGS = -W -Wall -g

mpi_sort : $(OBJS) mpi_sort.c
	$(CC) $(FLAGS) $(OBJS) mpi_sort.c -o mpi_sort

int_list.o : int_list.c
	$(CC) $(FLAGS) -c int_list.c

sort.o : sort.c
	$(CC) $(FLAGS) -c sort.c

clean:
	\rm *.o *~ mpi_sort
