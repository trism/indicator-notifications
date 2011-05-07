
bin_PROGRAMS = indicator-datetime-preferences
libexec_PROGRAMS = indicator-datetime-service

indicator_datetime_service_SOURCES = \
	datetime-interface.c \
	datetime-interface.h \
	gen-datetime-service.xml.c \
	calendar-menu-item.c \
	calendar-menu-item.h \
	datetime-service.c \
	utils.c \
	utils.h \
	dbus-shared.h \
	settings-shared.h
indicator_datetime_service_CFLAGS = \
	-Wall \
	-Werror \
	$(SERVICE_CFLAGS) \
	-DTIMEZONE_FILE="\"/etc/timezone\"" \
	-DG_LOG_DOMAIN=\"Indicator-Datetime\"
indicator_datetime_service_LDADD = \
	$(SERVICE_LIBS)

datetimelibdir = $(INDICATORDIR)
datetimelib_LTLIBRARIES = libdatetime.la
libdatetime_la_SOURCES = \
	gen-datetime-service.xml.h \
	dbus-shared.h \
	settings-shared.h \
	utils.c \
	utils.h \
	indicator-datetime.c
libdatetime_la_CFLAGS = \
	$(INDICATOR_CFLAGS) \
	-Wall -Werror \
	-DG_LOG_DOMAIN=\"Indicator-Datetime\"
libdatetime_la_LIBADD = \
	$(INDICATOR_LIBS)
libdatetime_la_LDFLAGS = \
	-module \
	-avoid-version

indicator_datetime_preferences_SOURCES =\
	datetime-prefs.c \
	datetime-prefs-locations.c \
	datetime-prefs-locations.h \
	timezone-completion.c \
	timezone-completion.h \
	utils.c \
	utils.h \
	settings-shared.h
indicator_datetime_preferences_CFLAGS = \
	-Wall \
	-Werror \
	-I$(top_srcdir)/libmap \
	$(PREF_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\""
indicator_datetime_preferences_LDADD = \
	 $(top_builddir)/libmap/libmap.la \
	$(PREF_LIBS)

gen-%.xml.c: %.xml
	@echo "Building $@ from $<"
	@echo "const char * _$(subst -,_,$(subst .,_,$(basename $<))) = " > $@
	@sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	@echo ";" >> $@

gen-%.xml.h: %.xml
	@echo "Building $@ from $<"
	@echo "extern const char * _$(subst -,_,$(subst .,_,$(basename $<)));" > $@

BUILT_SOURCES = \
	gen-datetime-service.xml.c \
	gen-datetime-service.xml.h


CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	datetime-service.xml
