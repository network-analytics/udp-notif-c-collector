AC_INIT([unyte_udp_notif], [0.4.0], [alex.huang-feng@insa-lyon.fr])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([/usr/local])

AM_PROG_AR
LT_INIT

AC_LANG([C])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC([gcc])

AC_PROG_LN_S

AC_ARG_ENABLE([tcmalloc],
    [AS_HELP_STRING([--enable-tcmalloc], [Use tcmalloc instead of native malloc. tcmalloc should be installed first.])],
    [ tcmalloc_enabled=yes ]
)

AC_ARG_WITH([examples],
    [AS_HELP_STRING([--with-examples], [Build examples samples])],
    [with_examples=yes],
    [with_examples=no]
)

AM_CONDITIONAL(EXAMPLES, test "x$with_examples" = "xyes")

AC_ARG_WITH([test],
    [AS_HELP_STRING([--with-test], [Build test files])],
    [with_test_files=yes],
    [with_test_files=no]
)

AM_CONDITIONAL(TEST_FILES, test "x$with_test_files" = "xyes")

if test "x$tcmalloc_enabled" = "xyes"
then
    AC_CHECK_LIB(tcmalloc, malloc, [], [
        echo "Error! You need to have tcmalloc first."
        exit -1
    ])
    EXTRA_LDFLAGS="-ltcmalloc -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free"
    AC_SUBST([EXTRA_LDFLAGS])
fi

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    examples/Makefile
    test/Makefile
    unyte-udp-notif.pc
])
AC_OUTPUT
