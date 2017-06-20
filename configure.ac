AC_INIT([bscand], [1.0])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE

AC_CHECK_LIB([sane], [sane_init])
AC_CHECK_LIB([cups], [cupsPrintFiles])
PKG_CHECK_MODULES([LIBCONFIG], [libconfig >= 1.5])

AC_PROG_CC

AC_OUTPUT(Makefile src/Makefile)
