CC=gcc
NVCC=nvcc
CFLAGS= -std=gnu99 -O0 # -Wall -Werror
NVFLAGS= -O0
LDFLAGS= -lpthread
DEBUG= #-ggdb
TARGET=cpu memory network disk gpu
OBJECTS= 
DEPS=benchmarking_tools.h

.PHONY: clean

all: $(TARGET)

cpu: cpu.o $(OBJECTS)
	$(CC) $(DEBUG) -o $@ $^ $(CFLAGS) $(LDFLAGS)

disk: disk_benchmark.o $(OBJECTS)
	$(CC) $(DEBUG) -o $@ $^ $(CFLAGS) $(LDFLAGS)

gpu: gpu.cu 
	$(NVCC) -o $@ $< $(NVFLAGS)

network: network.o $(OBJECTS)
	$(CC) $(DEBUG) -o $@ $^ $(CFLAGS) $(LDFLAGS)

memory: memory.o $(OBJECTS)
	$(CC) $(DEBUG) -o $@ $^ $(CFLAGS) $(LDFLAGS)

%.o: %.c $(DEPS)
	$(CC) $(DEBUG) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

clean: 
	rm -f *.o $(TARGET)
