SUBDIRS = icons

gsettings_file = net.launchpad.indicator.notifications.gschema.xml
gsettings_SCHEMAS = $(gsettings_file)

$(gsettings_file).in: $(gsettings_file).in.in
	$(AM_V_GEN) $(SED) \
	  -e "s|\@GETTEXT_PACKAGE\@|$(GETTEXT_PACKAGE)|" \
	  $< > $@

$(gsettings_file): $(gsettings_file).in
	$(AM_V_GEN) $(MSGFMT) --xml -d $(top_srcdir)/po \
		--template $< -o $@

@GSETTINGS_RULES@

EXTRA_DIST = \
	$(gsettings_file).in.in

CLEANFILES = \
	$(gsettings_file) \
	$(gsettings_file).in \
	*.gschema.valid
