AM_LDFLAGS = $(EXTRA_LDFLAGS)

bin_PROGRAMS = test_listener test_malloc test_queue test_segmentation test_version

test_listener_SOURCES = test_listener.c
test_listener_CFLAGS = -pthread -g
test_listener_LDADD = ../src/libunyte-udp-notif.la

test_malloc_SOURCES = test_malloc.c
test_malloc_CFLAGS = -pthread -g
test_malloc_LDADD = ../src/libunyte-udp-notif.la

test_queue_SOURCES = test_queue.c
test_queue_CFLAGS = -pthread -g
test_queue_LDADD = ../src/libunyte-udp-notif.la

test_segmentation_SOURCES = test_segmentation.c
test_segmentation_CFLAGS = -pthread -g
test_segmentation_LDADD = ../src/libunyte-udp-notif.la

test_version_SOURCES = test_version.c
test_version_CFLAGS = -pthread -g
test_version_LDADD = ../src/libunyte-udp-notif.la
