
CC=gcc
CFLAGS=-Wall


BINS=mythreads test

all: $(BINS)

mythreads: mythreads.c
	$(CC) $(CFLAGS) -c mythreads.c
	ar -cvr libmythreads.a mythreads.o

test: manycooperative_test.c manypreemptive_test.c
	$(CC) $(CFLAGS) -o manycooperative_test manycooperative_test.c libmythreads.a
	$(CC) $(CFLAGS) -o manypreemptive_test manypreemptive_test.c libmythreads.a
