#SUBDIRS = icons

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = indicator-datetime.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = \
	indicator-datetime.service.in

CLEANFILES = indicator-datetime.service
