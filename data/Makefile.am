#SUBDIRS = icons

gsettings_SCHEMAS = \
	com.canonical.indicator.datetime.gschema.xml
@GSETTINGS_RULES@

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = indicator-datetime.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

pkgdata_DATA = datetime-dialog.ui

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_DATA = indicator-datetime-preferences.desktop

EXTRA_DIST = \
	$(gsettings_SCHEMAS) \
	datetime-dialog.ui \
	indicator-datetime-preferences.desktop \
	indicator-datetime-preferences.desktop.in \
	indicator-datetime.service.in

CLEANFILES = indicator-datetime-preferences.desktop indicator-datetime.service
