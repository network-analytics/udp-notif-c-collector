CC=gcc
LDFLAGS=-g
CFLAGS= -Wextra -Wall -ansi -g -std=c99
DEPS= listening_worker.h hexdump.h unyte_utils.h queue.h parsing_worker.h unyte.h
OBJ= listening_worker.o hexdump.o unyte_utils.o queue.o parsing_worker.o unyte.o

all: client_sample test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client_sample: client_sample.o $(OBJ)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test: test.o $(OBJ)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

clean:
	rm *.o
