AM_LDFLAGS = $(EXTRA_LDFLAGS)

ACLOCAL_AMFLAGS = -I m4

LINUX_SRC=$(LINUX_HEADERS)
LINUX_H=-I$(LINUX_SRC)/arch/x86/include -I$(LINUX_SRC)/arch/x86/include/generated -I$(LINUX_SRC)/include -I$(LINUX_SRC)/arch/x86/include/uapi -I$(LINUX_SRC)/arch/x86/include/generated/uapi -I$(LINUX_SRC)/include/uapi -I$(LINUX_SRC)/include/generated/uapi
EBPF_CLDFLAGS=-fcolor-diagnostics -D_FILE_OFFSET_BITS=64 -Wall -Winvalid-pch -g -fPIC -g -O2 -D__KERNEL__ -D__TARGET_ARCH_x86 --target=bpf -Wall -Wno-macro-redefined -D__BPF_TRACING__

vmlinux.h:
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > $@

reuseport_udp_kern.o: vmlinux.h
	clang $(LINUX_H) $(EBPF_CLDFLAGS) -o $@ -c reuseport_udp_kern.c

all-local: reuseport_udp_kern.o

reuseport_udp_kern_odir = $(libdir)
reuseport_udp_kern_o_DATA = \
	reuseport_udp_kern.o \
	$(NULL)

bin_PROGRAMS = client_ebpf_user

client_ebpf_user_SOURCES = client_ebpf_user.c
client_ebpf_user_CFLAGS = -I$(srcdir)/../../src -Wextra -Wall -ansi -g -std=c11 -D_GNU_SOURCE -fPIC -pthread
client_ebpf_user_LDADD = $(srcdir)/../../src/libunyte-udp-notif.la
