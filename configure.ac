AC_INIT([indicator-notifications], [0.3.3], [jason.conti@gmail.com])

AM_INIT_AUTOMAKE([-Wall -Werror])

AM_SILENT_RULES([yes])

# Required compilers

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_AR
AC_STDC_HEADERS
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Setup gettext with intltool

IT_PROG_INTLTOOL
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package])
AC_SUBST(GETTEXT_PACKAGE)

# Helpers for GSettings

GLIB_GSETTINGS

# Required libraries from pkg-config

INDICATOR_REQUIRED_VERSION=0.3.19
GTK3_REQUIRED_VERSION=3.0

INDICATOR3_PKG_NAME=indicator3-0.4

PKG_CHECK_MODULES(INDICATOR, $INDICATOR3_PKG_NAME >= $INDICATOR_REQUIRED_VERSION
                             gtk+-3.0 >= GTK3_REQUIRED_VERSION)

AC_SUBST(INDICATOR_CFLAGS)
AC_SUBST(INDICATOR_LIBS)

# Library directories from pkg-config

with_localinstall="no"
AC_ARG_ENABLE([localinstall],
              [AS_HELP_STRING([--enable-localinstall],
                  [set default library directories instead of asking pkg-config (default is no)])],
              [with_localinstall="yes"],
              [with_localinstall="no"])

if test "x$with_localinstall" = "xyes"; then
  INDICATORDIR="${libdir}/indicators/2/"
else
  INDICATORDIR=`$PKG_CONFIG --variable=indicatordir $INDICATOR3_PKG_NAME`
fi
INDICATORICONSDIR="${datadir}/pixmaps/"

AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  data/Makefile
  data/icons/Makefile
  po/Makefile.in
  tests/Makefile
])

AC_OUTPUT

AC_MSG_NOTICE([

Recent Notifications Indicator Configuration:

	Prefix:        $prefix
	Indicator Dir: $INDICATORDIR
])
