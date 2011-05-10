#SUBDIRS = icons

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = indicator-notifications.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = \
	indicator-notifications.service.in

CLEANFILES = indicator-notifications.service
