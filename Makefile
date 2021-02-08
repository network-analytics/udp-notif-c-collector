CC=gcc
LDFLAGS=-g
CFLAGS= -Wextra -Wall -ansi -g -std=c99 -D_GNU_SOURCE
DEPS= listening_worker.h hexdump.h unyte_utils.h queue.h parsing_worker.h unyte.h
OBJ= listening_worker.o hexdump.o unyte_utils.o queue.o parsing_worker.o unyte.o

all: client_sample test client_performance client_loss

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client_sample: client_sample.o $(OBJ)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test: test.o $(OBJ)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

client_performance: client_performance.o $(OBJ)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

client_loss: client_loss.o $(OBJ)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

clean:
	rm *.o client_sample test client_performance client_loss
