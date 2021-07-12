AM_LDFLAGS = $(EXTRA_LDFLAGS)

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = client_monitoring client_sample sender_json sender_sample

client_monitoring_SOURCES = client_monitoring.c
client_monitoring_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
client_monitoring_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

client_sample_SOURCES = client_sample.c
client_sample_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
client_sample_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

sender_json_SOURCES = sender_json.c
sender_json_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
sender_json_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

sender_sample_SOURCES = sender_sample.c
sender_sample_CFLAGS = -I$(srcdir)/../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
sender_sample_LDADD = $(srcdir)/../src/libunyte-udp-notif.la

all-local:
	@if [ ! -L $(builddir)/resources ]; then\
		$(LN_S) -n $(top_builddir)/resources $(builddir)/resources ;\
	fi

clean-local:
	@if [ -L $(builddir)/resources ]; then\
		rm $(builddir)/resources ;\
	fi
