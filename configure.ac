
AC_INIT(indicator-datetime, 0.2.3, ted@canonical.com)
AC_COPYRIGHT([Copyright 2009,2010 Canonical])

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(indicator-datetime, 0.2.3)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.35.0])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ARG_ENABLE([deprecations],
  [AS_HELP_STRING([--enable-deprecations],
    [allow deprecated API usage @<:@default=yes@:>@])],
  [],
  [enable_deprecations=yes])
AS_IF([test "x$enable_deprecations" = xno],
  [CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE -DGTK_DISABLE_SINGLE_INCLUDES"]
)

AC_PATH_PROG([GLIB_MKENUMS], [glib-mkenums])
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])

PKG_PROG_PKG_CONFIG

###########################
# GTK+ version option
###########################

AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk],
    [Which version of gtk to use @<:@default=2@:>@])],
  [],
  [with_gtk=2])

###########################
# Dependencies
###########################

INDICATOR_REQUIRED_VERSION=0.3.19
DBUSMENUGLIB_REQUIRED_VERSION=0.1.1
DBUSMENUGTK_REQUIRED_VERSION=0.3.94
GIO_REQUIRED_VERSION=2.25.11
# Note: the GIO check below also ensures the proper glib with gsettings support is present
INDICATOR_DISPLAY_OBJECTS=0.2.2
GEOCLUE_REQUIRED_VERSION=0.12.0
ECAL_REQUIRED_VERSION=2.30
EDS_REQUIRED_VERSION=2.30
ICAL_REQUIRED_VERSION=0.44
CAIRO_REQUIRED_VERSION=1.10
GDK_REQUIRED_VERSION=2.22
GLIB_REQUIRED_VERSION=2.26
GTK_REQUIRED_VERSION=2.12
GTK3_REQUIRED_VERSION=3.0
GCONF_REQUIRED_VERSION=2.31

AS_IF([test "x$with_gtk" = x3],
    [PKG_CHECK_MODULES(INDICATOR, indicator3 >= $INDICATOR_REQUIRED_VERSION
                                  glib-2.0 >= $GLIB_REQUIRED_VERSION
                                  dbusmenu-glib-0.4 >= $DBUSMENUGLIB_REQUIRED_VERSION
                                  dbusmenu-gtk3-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
                                  libido3-0.1 >= $INDICATOR_DISPLAY_OBJECTS)
        ],
      [test "x$with_gtk" = x2],
    [PKG_CHECK_MODULES(INDICATOR, indicator >= $INDICATOR_REQUIRED_VERSION
                                  glib-2.0 >= $GLIB_REQUIRED_VERSION
                                  dbusmenu-glib-0.4 >= $DBUSMENUGLIB_REQUIRED_VERSION
                                  dbusmenu-gtk-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
                                  libido-0.1 >= $INDICATOR_DISPLAY_OBJECTS)
        ],
    [AC_MSG_FAILURE([Value for --with-gtk was neither 2 nor 3])]
)

AS_IF([test "x$with_gtk" = x3],
	[PKG_CHECK_MODULES(SERVICE,  indicator >= $INDICATOR_REQUIRED_VERSION
                                 glib-2.0 >= $GLIB_REQUIRED_VERSION
	                             dbusmenu-glib-0.4 >= $DBUSMENUGLIB_REQUIRED_VERSION
                                 dbusmenu-gtk3-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
	                             libido-0.1 >= $INDICATOR_DISPLAY_OBJECTS
	                             gio-2.0 >= $GIO_REQUIRED_VERSION
	                             geoclue >= $GEOCLUE_REQUIRED_VERSION
	                             libecal-1.2 >= $ECAL_REQUIRED_VERSION
	                             libical >= $ICAL_REQUIRED_VERSION
	                             libedataserver-1.2 >= EDS_REQUIRED_VERSION
	                             libedataserverui-1.2 >= EDS_REQUIRED_VERSION
	                             cairo >= CAIRO_REQUIRED_VERSION
	                             gdk-2.0 >= GDK_REQUIRED_VERSION
	                             gconf-2.0 >= GCONF_REQUIRED_VERSION)
		],
      [test "x$with_gtk" = x2],
	[PKG_CHECK_MODULES(SERVICE,  indicator >= $INDICATOR_REQUIRED_VERSION
                                 glib-2.0 >= $GLIB_REQUIRED_VERSION
	                             dbusmenu-glib-0.4 >= $DBUSMENUGLIB_REQUIRED_VERSION
                                 dbusmenu-gtk-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
	                             libido-0.1 >= $INDICATOR_DISPLAY_OBJECTS
	                             gio-2.0 >= $GIO_REQUIRED_VERSION
	                             geoclue >= $GEOCLUE_REQUIRED_VERSION
	                             libecal-1.2 >= $ECAL_REQUIRED_VERSION
	                             libical >= $ICAL_REQUIRED_VERSION
	                             libedataserver-1.2 >= EDS_REQUIRED_VERSION
	                             libedataserverui-1.2 >= EDS_REQUIRED_VERSION
	                             cairo >= CAIRO_REQUIRED_VERSION
	                             gdk-2.0 >= GDK_REQUIRED_VERSION
	                             gconf-2.0 >= GCONF_REQUIRED_VERSION)
        ],
    [AC_MSG_FAILURE([Value for --with-gtk was neither 2 nor 3])]
)

# FIXME: polkit-gtk-1 isn't gtk3-compatible
AS_IF([test "x$with_gtk" = x3],
	[PKG_CHECK_MODULES(PREF, gio-2.0 >= $GIO_REQUIRED_VERSION
	                         gtk+-3.0 >= $GTK3_REQUIRED_VERSION
                                 unique-3.0
                                 json-glib-1.0
                                 polkit-gtk-1)
		],
      [test "x$with_gtk" = x2],
	[PKG_CHECK_MODULES(PREF, gio-2.0 >= $GIO_REQUIRED_VERSION
	                         gtk+-2.0 >= $GTK_REQUIRED_VERSION
                                 unique-1.0
                                 json-glib-1.0
                                 polkit-gtk-1)
        ],
    [AC_MSG_FAILURE([Value for --with-gtk was neither 2 nor 3])]
)

AS_IF([test "x$with_gtk" = x3],
	[PKG_CHECK_MODULES(LIBMAP, gio-2.0 >= $GIO_REQUIRED_VERSION
	                           gtk+-3.0 >= $GTK3_REQUIRED_VERSION)
		],
      [test "x$with_gtk" = x2],
	[PKG_CHECK_MODULES(LIBMAP, gio-2.0 >= $GIO_REQUIRED_VERSION
	                           gtk+-2.0 >= $GTK_REQUIRED_VERSION)
        ],
    [AC_MSG_FAILURE([Value for --with-gtk was neither 2 nor 3])]
)
AC_SUBST(INDICATOR_CFLAGS)
AC_SUBST(INDICATOR_LIBS)

AC_SUBST(SERVICE_CFLAGS)
AC_SUBST(SERVICE_LIBS)

AC_SUBST(PREF_CFLAGS)
AC_SUBST(PREF_LIBS)

AC_SUBST(LIBMAP_CFLAGS)
AC_SUBST(LIBMAP_LIBS)
AC_SUBST(LIBMAP_LDFLAGS)

###########################
# Grab the GSettings Macros
###########################

GLIB_GSETTINGS

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE(localinstall, AS_HELP_STRING([--enable-localinstall], [install all of the files localy instead of system directories (for distcheck)]), with_localinstall=$enableval, with_localinstall=no)

###########################
# Indicator Info
###########################

AS_IF([test "x$with_localinstall" = "xyes"],
	[
	INDICATORDIR="${libdir}/indicators/2/"
	INDICATORICONSDIR="${datadir}/libindicate/icons/"
	],
	[AS_IF([test "x$with_gtk" = "x2"],
		[
		INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator`
		INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator`
		],
		[
		INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator3`
		INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator3`
		])])

AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

###########################
# DBus Service Info
###########################

if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

##############################
# Custom Junk
##############################

AC_DEFUN([AC_DEFINE_PATH], [
	test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
	ac_define_path=`eval echo [$]$2`
	ac_define_path=`eval echo [$]ac_define_path`
	$1="$ac_define_path"
	AC_SUBST($1)
	ifelse($3, ,
		AC_DEFINE_UNQUOTED($1, "$ac_define_path"),
		AC_DEFINE_UNQUOTED($1, "$ac_define_path", $3))
])

###########################
# Internationalization
###########################

GETTEXT_PACKAGE=indicator-datetime
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the default get text domain])
AC_DEFINE_PATH(GNOMELOCALEDIR, "${datadir}/locale", [locale directory])

AM_GLIB_GNU_GETTEXT

###########################
# Files
###########################

AC_OUTPUT([
Makefile
libmap/Makefile
src/Makefile
data/Makefile
tests/Makefile
po/Makefile.in
])

###########################
# Results
###########################

AC_MSG_NOTICE([

Date and Time Indicator Configuration:

	Prefix:        $prefix
	Indicator Dir: $INDICATORDIR
])
