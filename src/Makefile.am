AM_CPPFLAGS = -DICONS_DIR='"$(INDICATORICONSDIR)"'

notificationslibdir = $(INDICATORDIR)
notificationslib_LTLIBRARIES = libnotifications.la
libnotifications_la_SOURCES = \
	dbus-spy.c \
	dbus-spy.h \
	notification-menuitem.c \
	notification-menuitem.h \
	indicator-notifications.c \
	notification.c \
	notification.h
libnotifications_la_CFLAGS = \
	$(INDICATOR_CFLAGS) \
	-Wall \
	-DG_LOG_DOMAIN=\"Indicator-Notifications\"
libnotifications_la_LIBADD = \
	$(INDICATOR_LIBS)
libnotifications_la_LDFLAGS = \
	-module \
	-avoid-version

