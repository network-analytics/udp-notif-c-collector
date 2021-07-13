AM_LDFLAGS = $(EXTRA_LDFLAGS)

SUBDIRS = src

if EXAMPLES
SUBDIRS += examples
endif

if TEST_FILES
SUBDIRS += test
endif

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	unyte-udp-notif.pc \
	$(NULL)
