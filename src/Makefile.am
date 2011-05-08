
libexec_PROGRAMS = indicator-example-service

indicator_example_service_SOURCES = \
	example-interface.c \
	example-interface.h \
	gen-example-service.xml.c \
	example-service.c \
	dbus-shared.h \
	settings-shared.h
indicator_example_service_CFLAGS = \
	-Wall \
	$(SERVICE_CFLAGS) \
	-DTIMEZONE_FILE="\"/etc/timezone\"" \
	-DG_LOG_DOMAIN=\"Indicator-Example\"
indicator_example_service_LDADD = \
	$(SERVICE_LIBS)

examplelibdir = $(INDICATORDIR)
examplelib_LTLIBRARIES = libexample.la
libexample_la_SOURCES = \
	gen-example-service.xml.h \
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

gen-%.xml.c: %.xml
	@echo "Building $@ from $<"
	@echo "const char * _$(subst -,_,$(subst .,_,$(basename $<))) = " > $@
	@sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	@echo ";" >> $@

gen-%.xml.h: %.xml
	@echo "Building $@ from $<"
	@echo "extern const char * _$(subst -,_,$(subst .,_,$(basename $<)));" > $@

BUILT_SOURCES = \
	gen-example-service.xml.c \
	gen-example-service.xml.h


CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	example-service.xml
