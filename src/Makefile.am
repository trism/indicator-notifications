notificationslibdir = $(INDICATORDIR)

notificationslib_LTLIBRARIES = libnotifications.la

libnotifications_la_SOURCES = \
	dbus-spy.c \
	dbus-spy.h \
	urlregex.c \
	urlregex.h \
	notification-menuitem.c \
	notification-menuitem.h \
	settings.h \
	indicator-notifications.c \
	notification.c \
	notification.h

libnotifications_la_CFLAGS = \
	-DSETTINGS_PATH=\""$(libexecdir)/$(PACKAGE)/indicator-notifications-settings"\" \
	$(INDICATOR_CFLAGS) \
	-Wall \
	-DG_LOG_DOMAIN=\"Indicator-Notifications\"

libnotifications_la_LIBADD = \
	$(INDICATOR_LIBS)

libnotifications_la_LDFLAGS = \
	-module \
	-avoid-version

pkglibexec_PROGRAMS = indicator-notifications-settings

indicator_notifications_settings_SOURCES = \
	settings.h \
	indicator-notifications-settings.c

indicator_notifications_settings_CFLAGS = \
	-DLOCALE_DIR=\""$(localedir)"\" \
	$(SETTINGS_CFLAGS) \
	-Wall

indicator_notifications_settings_LDADD = \
	$(SETTINGS_LIBS)
