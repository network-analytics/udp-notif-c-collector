#/bin/bash

# Removes all autotools created files
rm Makefile.in configure config.h.in aclocal.m4 config.h config.log libtool stamp-h1 unyte-udp-notif.pc config.status
rm -r build
rm -r m4
rm -r build-aux
rm -r autom4te.cache
rm Makefile
rm -r src/Makefile.in src/Makefile src/.deps
rm -r test/Makefile.in test/Makefile test/.deps
rm -r examples/Makefile.in examples/Makefile examples/.deps examples/.libs examples/*.o
rm src/*.o src/*.lo
rm -r src/.libs
rm src/*.la