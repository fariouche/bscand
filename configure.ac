AC_INIT([bscand], [1.0])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE

AC_CHECK_LIB([sane], [sane_init])
AC_CHECK_LIB([cups], [cupsPrintFiles])
PKG_CHECK_MODULES([LIBCONFIG], [libconfig >= 1.4.9])

AC_PROG_CC

AC_ARG_ENABLE([ipv6],
    AS_HELP_STRING([--enable-ipv6], [enable ipv6 support]))
AS_IF(
      [test "x$enable_ipv6" = "xyes"],
      [
       AC_DEFINE([ENABLE_IPV6])
      ]
)


AC_OUTPUT(Makefile src/Makefile)
