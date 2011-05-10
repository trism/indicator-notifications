
libexec_PROGRAMS = indicator-example-service

indicator_example_service_SOURCES = \
	example-service.c \
	dbus-shared.h \
	settings-shared.h
indicator_example_service_CFLAGS = \
	-Wall \
	$(SERVICE_CFLAGS) \
	-DG_LOG_DOMAIN=\"Indicator-Example\"
indicator_example_service_LDADD = \
	$(SERVICE_LIBS)

examplelibdir = $(INDICATORDIR)
examplelib_LTLIBRARIES = libexample.la
libexample_la_SOURCES = \
	dbus-shared.h \
	settings-shared.h \
	indicator-example.c
libexample_la_CFLAGS = \
	$(INDICATOR_CFLAGS) \
	-Wall \
	-DG_LOG_DOMAIN=\"Indicator-Example\"
libexample_la_LIBADD = \
	$(INDICATOR_LIBS)
libexample_la_LDFLAGS = \
	-module \
	-avoid-version

