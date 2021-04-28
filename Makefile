###### GCC options ######
CC=gcc
# LDFLAGS=-g -L./ -lunyte-udp-notif
LDFLAGS=-g
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
_OBJS=hexdump.o listening_worker.o unyte_udp_utils.o unyte_udp_queue.o parsing_worker.o unyte_collector.o segmentation_buffer.o cleanup_worker.o unyte_sender.o monitoring_worker.o
OBJS=$(patsubst %,$(ODIR)/%,$(_OBJS))

###### c-collector source headers ######
_DEPS=hexdump.h listening_worker.h unyte_udp_utils.h unyte_udp_queue.h parsing_worker.h unyte_collector.h segmentation_buffer.h cleanup_worker.h unyte_sender.h monitoring_worker.h
DEPS=$(patsubst %,$(SDIR)/%,$(_DEPS))

###### c-collector examples ######
SAMPLES_DIR=samples
SAMPLES_ODIR=samples/obj

###### c-collector test files ######
TDIR=test

BINS=client_sample sender_sample sender_json client_monitoring
TESTBINS=test_malloc test_queue test_seg test_listener test_version

all: libunyte-udp-notif.so $(BINS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(SAMPLES_ODIR)/%.o: $(SAMPLES_DIR)/%.c 
	$(CC) -c -o $@ $< $(CFLAGS)

libunyte-udp-notif.so: $(OBJS)
	$(CC) -shared -o libunyte-udp-notif.so $(OBJS)

client_sample: $(SAMPLES_ODIR)/client_sample.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

sender_sample: $(SAMPLES_ODIR)/sender_sample.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

sender_json: $(SAMPLES_ODIR)/sender_json.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

client_monitoring: $(SAMPLES_ODIR)/client_monitoring.o $(OBJS)
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

test_version: $(TDIR)/test_version.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

install: libunyte-udp-notif.so
	./install.sh

build: libunyte-udp-notif.so

clean:
	rm $(ODIR)/*.o $(SAMPLES_ODIR)/*.o $(TDIR)/*.o $(BINS) $(TESTBINS) libunyte-udp-notif.so
