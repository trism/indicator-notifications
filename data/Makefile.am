#SUBDIRS = icons

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = indicator-example.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = \
	indicator-example.service.in

CLEANFILES = indicator-example.service
