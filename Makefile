###### GCC options ######
CC=gcc
LDFLAGS=-g -L./ -lunyte-udp-notif
CFLAGS=-Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC

## TCMALLOCFLAGS for tcmalloc
TCMALLOCFLAGS=-ltcmalloc -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
TCMALLOCFLAGS=

## For test third parties lib
USE_LIB=$(shell pkg-config --cflags --libs unyte-udp-notif)
USE_LIB=

###### c-collector source code ######
SDIR=src
ODIR=obj
_OBJS=hexdump.o listening_worker.o unyte_utils.o queue.o parsing_worker.o unyte_collector.o segmentation_buffer.o cleanup_worker.o unyte_sender.o
OBJS=$(patsubst %,$(ODIR)/%,$(_OBJS))

###### c-collector source headers ######
_DEPS=hexdump.h listening_worker.h unyte_utils.h queue.h parsing_worker.h unyte_collector.h segmentation_buffer.h cleanup_worker.h unyte_sender.h
DEPS=$(patsubst %,$(SDIR)/%,$(_DEPS))

###### c-collector examples ######
SAMPLES_DIR=samples
SAMPLES_ODIR=samples/obj

###### c-collector test files ######
TDIR=test

BINS=client_sample client_performance client_loss sender_sample sender_json sender_performance sender_continuous
TESTBINS=test_malloc test_queue test_seg test_listener

all: libunyte-udp-notif.so $(BINS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(LODIR)/%.o: $(LDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(SAMPLES_ODIR)/%.o: $(SAMPLES_DIR)/%.c 
	$(CC) -c -o $@ $< $(CFLAGS)

libunyte-udp-notif.so: $(OBJS)
	$(CC) -shared -o libunyte-udp-notif.so $(OBJS)

client_sample: $(SAMPLES_ODIR)/client_sample.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

client_performance: $(SAMPLES_ODIR)/client_performance.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

client_loss: $(SAMPLES_ODIR)/client_loss.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

sender_sample: $(SAMPLES_ODIR)/sender_sample.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

sender_json: $(SAMPLES_ODIR)/sender_json.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

sender_performance: $(SAMPLES_ODIR)/sender_performance.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

sender_continuous: $(SAMPLES_ODIR)/sender_continuous.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

## own test files
test_listener: $(TDIR)/test_listener.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test_seg: $(TDIR)/test_segmentation.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test_queue: $(TDIR)/test_queue.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test_malloc: $(TDIR)/test_malloc.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

install: libunyte-udp-notif.so
	./install.sh

build: libunyte-udp-notif.so

clean:
	rm $(ODIR)/*.o $(SAMPLES_ODIR)/*.o $(TDIR)/*.o $(BINS) $(TESTBINS) libunyte-udp-notif.so
