XEN_ROOT="/root/xen-4.1.2"

all: main.o numa_scheduler.o
	gcc -g -o run.bin main.o numa_scheduler.o -lxenctrl
main.o: main.c
	gcc -g -c main.c
numa_scheduler.o: numa_scheduler.c numa_scheduler.h
	gcc -g -c numa_scheduler.c 
clean:
	rm run.bin *.o

