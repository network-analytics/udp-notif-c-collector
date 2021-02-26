###### GCC options ######
CC=gcc
LDFLAGS=-g
CFLAGS= -Wextra -Wall -ansi -g -std=c99 -D_GNU_SOURCE -fPIC

###### c-collector source code ######
SDIR = src
ODIR = obj
_OBJS = hexdump.o listening_worker.o unyte_utils.o queue.o parsing_worker.o unyte_collector.o segmentation_buffer.o cleanup_worker.o unyte_sender.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

###### c-collector source headers ######
_DEPS = hexdump.h listening_worker.h unyte_utils.h queue.h parsing_worker.h unyte_collector.h segmentation_buffer.h cleanup_worker.h unyte_sender.h
DEPS = $(patsubst %,$(SDIR)/%,$(_DEPS))

###### c-collector examples ######
SAMPLES_DIR = samples
SAMPLES_ODIR = samples/obj

###### c-collector test files ######
TDIR = test

BINS = client_sample client_performance client_loss test_listener test_seg sender_sample sender_json sender_performance test_queue

all: $(BINS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

$(LODIR)/%.o: $(LDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) 

$(SAMPLES_ODIR)/%.o: $(SAMPLES_DIR)/%.c 
	$(CC) -c -o $@ $< $(CFLAGS) 

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

test_listener: $(TDIR)/test.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test_seg: $(TDIR)/test_segmentation.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test_queue: $(TDIR)/test_queue.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

build: 
	$(CC) -shared -o libunyte.so $(OBJS)

clean:
	rm $(ODIR)/*.o $(SAMPLES_ODIR)/*.o $(TDIR)/*.o $(BINS)
