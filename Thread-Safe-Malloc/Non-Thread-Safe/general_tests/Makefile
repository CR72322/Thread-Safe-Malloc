CC=gcc
CFLAGS=-O3 -fPIC -DDEBUG
MALLOC_VERSION=BF
WDIR=/home/yy379/ece551/ECE650_hw1_kit_2

all: mymalloc_test

mymalloc_test: mymalloc_test.c
	$(CC) $(CFLAGS) -I$(WDIR) -L$(WDIR) -D$(MALLOC_VERSION) -Wl,-rpath=$(WDIR) -o $@ mymalloc_test.c -lmymalloc -lrt

clean:
	rm -f *~ *.o mymalloc_test

clobber:
	rm -f *~ *.o
