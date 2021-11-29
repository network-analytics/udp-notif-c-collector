AM_LDFLAGS = $(EXTRA_LDFLAGS)

if EBPF_EXAMPLE
SUBDIRS = eBPF
endif

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = client_monitoring client_sample client_socket sender_json sender_sample sender_custom_encoding client_legacy_proto sender_cbor

client_monitoring_SOURCES = client_monitoring.c
client_monitoring_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
client_monitoring_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

client_sample_SOURCES = client_sample.c
client_sample_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
client_sample_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

client_socket_SOURCES = client_socket.c
client_socket_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
client_socket_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

sender_json_SOURCES = sender_json.c
sender_json_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
sender_json_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

sender_sample_SOURCES = sender_sample.c
sender_sample_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
sender_sample_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

sender_custom_encoding_SOURCES = sender_custom_encoding.c
sender_custom_encoding_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
sender_custom_encoding_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

client_legacy_proto_SOURCES = client_legacy_proto.c
client_legacy_proto_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
client_legacy_proto_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

sender_cbor_SOURCES = sender_cbor.c
sender_cbor_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
sender_cbor_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

all-local:
	@if [ ! -L $(builddir)/resources ]; then\
		$(LN_S) -n $(top_builddir)/resources $(builddir)/resources ;\
	fi

clean-local:
	@if [ -L $(builddir)/resources ]; then\
		rm $(builddir)/resources ;\
	fi
