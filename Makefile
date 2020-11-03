CC=gcc
LDFLAGS=-g
CFLAGS= -Wextra -Wall -ansi -g
DEPS= listening_worker.h hexdump.h unyte_utils.h
OBJ= listening_worker.o hexdump.o unyte_utils.o

all: main test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: main.o $(OBJ)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test: test.o $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm *.o