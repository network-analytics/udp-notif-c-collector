AM_LDFLAGS = $(EXTRA_LDFLAGS) -I/usr/local/include -L/usr/local/lib -lwolfssl -levent -levent_core

lib_LTLIBRARIES = libunyte-udp-notif.la

libunyte_udp_notif_la_SOURCES = \
    cleanup_worker.c            \
    cleanup_worker.h            \
    hexdump.c                   \
    hexdump.h                   \
    listening_worker.c          \
    listening_worker.h          \
    monitoring_worker.c         \
    monitoring_worker.h         \
    parsing_worker.c            \
    parsing_worker.h            \
    segmentation_buffer.c       \
    segmentation_buffer.h       \
    unyte_sender.c              \
    unyte_sender.h              \
    unyte_udp_collector.c       \
    unyte_udp_collector.h       \
    unyte_udp_queue.c           \
    unyte_udp_queue.h           \
    unyte_udp_utils.c           \
    unyte_udp_utils.h           \
    unyte_version.h             \
    unyte_udp_defaults.h        \
    unyte_udp_constants.h       \
    $(NULL)

unyteincludedir = $(includedir)/unyte-udp-notif
unyteinclude_HEADERS =    \
    cleanup_worker.h      \
    hexdump.h             \
    listening_worker.h    \
    monitoring_worker.h   \
    parsing_worker.h      \
    segmentation_buffer.h \
    unyte_sender.h        \
    unyte_udp_collector.h \
    unyte_udp_queue.h     \
    unyte_udp_utils.h     \
    unyte_version.h       \
    unyte_udp_defaults.h  \
    unyte_udp_constants.h \
    $(NULL)

libunyte_udp_notif_la_CFLAGS = -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC
