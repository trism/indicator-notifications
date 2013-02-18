SUBDIRS = icons

gsettings_file = net.launchpad.indicator.notifications.gschema.xml
gsettings_SCHEMAS = $(gsettings_file)

@INTLTOOL_XML_NOMERGE_RULE@

$(gsettings_file).in: $(gsettings_file).in.in
	sed \
	  -e "s|\@GETTEXT_PACKAGE\@|$(GETTEXT_PACKAGE)|" \
	  $< > $@

@GSETTINGS_RULES@

EXTRA_DIST = \
	$(gsettings_file).in.in

CLEANFILES = \
	$(gsettings_file) \
	$(gsettings_file).in \
	*.gschema.valid
