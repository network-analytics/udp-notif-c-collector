###### GCC options ######
CC=gcc
LDFLAGS=-g
CFLAGS= -Wextra -Wall -ansi -g -std=c99 -D_GNU_SOURCE

###### c-collector source code ######
SDIR = src
ODIR = obj
_OBJS = listening_worker.o unyte_utils.o queue.o parsing_worker.o unyte.o segmentation_buffer.o cleanup_worker.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

###### c-collector source headers ######
_DEPS = listening_worker.h unyte_utils.h queue.h parsing_worker.h unyte.h segmentation_buffer.h cleanup_worker.h
DEPS = $(patsubst %,$(SDIR)/%,$(_DEPS))

###### c-collector lib source code ######
LDIR = src/lib
LODIR = obj/lib
_LIBS = hexdump.o
LIBS = $(patsubst %,$(LODIR)/%,$(_LIBS))

###### c-collector lib headers ######
_LDEPS = hexdump.h
LDEPS = $(patsubst %,$(LDIR)/%,$(_LDEPS))

###### c-collector examples ######
SAMPLES_DIR = samples
SAMPLES_ODIR = samples/obj

###### c-collector test files ######
TDIR = test

all: client_sample client_performance client_loss test_listener test_seg

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) $(LDEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

$(LODIR)/%.o: $(LDIR)/%.c $(LDEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

$(SAMPLES_ODIR)/%.o: $(SAMPLES_DIR)/%.c 
	$(CC) -c -o $@ $< $(CFLAGS) 

client_sample: $(SAMPLES_ODIR)/client_sample.o $(OBJS) $(LIBS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

client_performance: $(SAMPLES_ODIR)/client_performance.o $(OBJS) $(LIBS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

client_loss: $(SAMPLES_ODIR)/client_loss.o $(OBJS) $(LIBS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test_listener: $(TDIR)/test.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

test_seg: $(TDIR)/test_segmentation.o $(OBJS)
	$(CC) -pthread -o $@ $^ $(LDFLAGS)

clean:
	rm $(ODIR)/*.o $(SAMPLES_ODIR)/*.o $(TDIR)/*.o $(LODIR)/*.o client_sample client_loss client_performance test_seg test_listener
