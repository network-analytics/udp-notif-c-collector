CC=gcc
LDFLAGS=-g
CFLAGS= -Wextra -Wall -ansi -g
DEPS= udp-list.h hexdump.h unyte_tools.h
OBJ= udp-list.o hexdump.o unyte_tools.o

all: main test

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: main.o $(OBJ)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test: test.o $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm *.o