AM_CPPFLAGS = -DICONS_DIR='"$(INDICATORICONSDIR)"' \
	      -DWITH_GTK='$(GTK_VERSION)'

libexec_PROGRAMS = indicator-notifications-service

indicator_notifications_service_SOURCES = \
	dbus-spy.c \
	dbus-spy.h \
	notification.c \
	notification.h \
	notifications-interface.c \
	notifications-interface.h \
	gen-notifications-service.xml.c \
	notifications-service.c \
	dbus-shared.h \
	settings-shared.h
indicator_notifications_service_CFLAGS = \
	-Wall \
	$(SERVICE_CFLAGS) \
	-DG_LOG_DOMAIN=\"Indicator-Notifications\"
indicator_notifications_service_LDADD = \
	$(SERVICE_LIBS)

notificationslibdir = $(INDICATORDIR)
notificationslib_LTLIBRARIES = libnotifications.la
libnotifications_la_SOURCES = \
	dbus-shared.h \
	settings-shared.h \
	indicator-notifications.c
libnotifications_la_CFLAGS = \
	$(INDICATOR_CFLAGS) \
	-Wall \
	-DG_LOG_DOMAIN=\"Indicator-Notifications\"
libnotifications_la_LIBADD = \
	$(INDICATOR_LIBS)
libnotifications_la_LDFLAGS = \
	-module \
	-avoid-version

gen-notifications-service.xml.c: notifications-service.xml
	@echo "Building $@ from $<"
	@echo "const char * _notifications_service = " > $@
	@sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	@echo ";" >> $@

gen-notifications-service.xml.h: notifications-service.xml
	@echo "Building $@ from $<"
	@echo "extern const char * _notifications_service;" > $@

BUILT_SOURCES = \
	gen-notifications-service.xml.c \
	gen-notifications-service.xml.h

CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	notifications-service.xml
