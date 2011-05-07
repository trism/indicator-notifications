/* -*- Mode: C; coding: utf-8; indent-tabs-mode: nil; tab-width: 2 -*-

A dialog for setting time and date preferences.

Copyright 2010 Canonical Ltd.

Authors:
    Michael Terry <michael.terry@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <locale.h>
#include <langinfo.h>
#include <string.h>
#include "utils.h"
#include "settings-shared.h"

/* Check the system locale setting to see if the format is 24-hour
   time or 12-hour time */
gboolean
is_locale_12h (void)
{
	static const char *formats_24h[] = {"%H", "%R", "%T", "%OH", "%k", NULL};
	const char *t_fmt = nl_langinfo (T_FMT);
	int i;

	for (i = 0; formats_24h[i]; ++i) {
		if (strstr (t_fmt, formats_24h[i])) {
			return FALSE;
		}
	}

	return TRUE;
}

void
split_settings_location (const gchar * location, gchar ** zone, gchar ** name)
{
  gchar * location_dup = g_strdup (location);
  gchar * first = strchr (location_dup, ' ');

  if (first) {
    first[0] = 0;
  }

  if (zone) {
    *zone = location_dup;
  }

  if (name) {
    gchar * after = first ? g_strstrip (first + 1) : NULL;
    if (after == NULL || after[0] == 0) {
      /* Make up name from zone */
      gchar * chr = strrchr (location_dup, '/');
      after = g_strdup (chr ? chr + 1 : location_dup);
      while ((chr = strchr (after, '_')) != NULL) { /* and turn underscores to spaces */
        *chr = ' ';
      }
      *name = after;
    }
    else {
      *name = g_strdup (after);
    }
  }
}

gchar *
get_current_zone_name (const gchar * location)
{
  gchar * new_zone, * new_name;
  gchar * old_zone, * old_name;
  gchar * rv;

  split_settings_location (location, &new_zone, &new_name);

  GSettings * conf = g_settings_new (SETTINGS_INTERFACE);
  gchar * tz_name = g_settings_get_string (conf, SETTINGS_TIMEZONE_NAME_S);
  split_settings_location (tz_name, &old_zone, &old_name);
  g_free (tz_name);
  g_object_unref (conf);

  // new_name is always just a sanitized version of a timezone.
  // old_name is potentially a saved "pretty" version of a timezone name from
  // geonames.  So we prefer to use it if available and the zones match.

  if (g_strcmp0 (old_zone, new_zone) == 0) {
    rv = old_name;
    old_name = NULL;
  }
  else {
    rv = new_name;
    new_name = NULL;
  }

  g_free (new_zone);
  g_free (old_zone);
  g_free (new_name);
  g_free (old_name);

  return rv;
}

/* Translate msg according to the locale specified by LC_TIME */
static char *
T_(const char *msg)
{
	/* General strategy here is to make sure LANGUAGE is empty (since that
	   trumps all LC_* vars) and then to temporarily swap LC_TIME and
	   LC_MESSAGES.  Then have gettext translate msg.

	   We strdup the strings because the setlocale & *env functions do not
	   guarantee anything about the storage used for the string, and thus
	   the string may not be portably safe after multiple calls.

	   Note that while you might think g_dcgettext would do the trick here,
	   that actually looks in /usr/share/locale/XX/LC_TIME, not the
	   LC_MESSAGES directory, so we won't find any translation there.
	*/
	char *message_locale = g_strdup(setlocale(LC_MESSAGES, NULL));
	char *time_locale = g_strdup(setlocale(LC_TIME, NULL));
	char *language = g_strdup(g_getenv("LANGUAGE"));
	char *rv;
	g_unsetenv("LANGUAGE");
	setlocale(LC_MESSAGES, time_locale);

	/* Get the LC_TIME version */
	rv = _(msg);

	/* Put everything back the way it was */
	setlocale(LC_MESSAGES, message_locale);
	g_setenv("LANGUAGE", language, TRUE);
	g_free(message_locale);
	g_free(time_locale);
	g_free(language);
	return rv;
}

/* Tries to figure out what our format string should be.  Lots
   of translator comments in here. */
gchar *
generate_format_string_full (gboolean show_day, gboolean show_date)
{
	gboolean twelvehour = TRUE;

	GSettings * settings = g_settings_new (SETTINGS_INTERFACE);
	gint time_mode = g_settings_get_enum (settings, SETTINGS_TIME_FORMAT_S);
	gboolean show_seconds = g_settings_get_boolean (settings, SETTINGS_SHOW_SECONDS_S);
	g_object_unref (settings);

	if (time_mode == SETTINGS_TIME_LOCALE) {
		twelvehour = is_locale_12h();
	} else if (time_mode == SETTINGS_TIME_24_HOUR) {
		twelvehour = FALSE;
	}

	const gchar * time_string = NULL;
	if (twelvehour) {
		if (show_seconds) {
			/* TRANSLATORS: A format string for the strftime function for
			   a clock showing 12-hour time with seconds. */
			time_string = T_("%l:%M:%S %p");
		} else {
			time_string = T_(DEFAULT_TIME_12_FORMAT);
		}
	} else {
		if (show_seconds) {
			/* TRANSLATORS: A format string for the strftime function for
			   a clock showing 24-hour time with seconds. */
			time_string = T_("%H:%M:%S");
		} else {
			time_string = T_(DEFAULT_TIME_24_FORMAT);
		}
	}
	
	/* Checkpoint, let's not fail */
	g_return_val_if_fail(time_string != NULL, g_strdup(DEFAULT_TIME_FORMAT));

	/* If there's no date or day let's just leave now and
	   not worry about the rest of this code */
	if (!show_date && !show_day) {
		return g_strdup(time_string);
	}

	const gchar * date_string = NULL;
	if (show_date && show_day) {
		/* TRANSLATORS:  This is a format string passed to strftime to represent
		   the day of the week, the month and the day of the month. */
		date_string = T_("%a %b %e");
	} else if (show_date) {
		/* TRANSLATORS:  This is a format string passed to strftime to represent
		   the month and the day of the month. */
		date_string = T_("%b %e");
	} else if (show_day) {
		/* TRANSLATORS:  This is a format string passed to strftime to represent
		   the day of the week. */
		date_string = T_("%a");
	}

	/* Check point, we should have a date string */
	g_return_val_if_fail(date_string != NULL, g_strdup(time_string));

	/* TRANSLATORS: This is a format string passed to strftime to combine the
	   date and the time.  The value of "%s %s" would result in a string like
	   this in US English 12-hour time: 'Fri Jul 16 11:50 AM' */
	return g_strdup_printf(T_("%s %s"), date_string, time_string);
}

gchar *
generate_format_string_at_time (GDateTime * time)
{
	/* This is a bit less free-form than for the main "now" time label. */
	/* If it is today, just the time should be shown (e.g. “3:55 PM”)
           If it is a different day this week, the day and time should be shown (e.g. “Wed 3:55 PM”)
           If it is after this week, the day, date, and time should be shown (e.g. “Wed 21 Apr 3:55 PM”). 
           In addition, when presenting the times of upcoming events, the time should be followed by the timezone if it is different from the one the computer is currently set to. For example, “Wed 3:55 PM UTC−5”. */
	gboolean show_day = FALSE;
	gboolean show_date = FALSE;

	GDateTime * now = g_date_time_new_now_local();

	/* First, are we same day? */
	gint time_year, time_month, time_day;
	gint now_year, now_month, now_day;
	g_date_time_get_ymd(time, &time_year, &time_month, &time_day);
	g_date_time_get_ymd(now, &now_year, &now_month, &now_day);

	if (time_year != now_year ||
	    time_month != now_month ||
	    time_day != now_day) {
		/* OK, different days so we must at least show the day. */
		show_day = TRUE;

		/* Is it this week? */
		/* Here, we define "is this week" as yesterday, today, or the next five days */
		GDateTime * past = g_date_time_add_days(now, -1);
		GDateTime * future = g_date_time_add_days(now, 5);
		GDateTime * past_bound = g_date_time_new_local(g_date_time_get_year(past),
		                                               g_date_time_get_month(past),
		                                               g_date_time_get_day_of_month(past),
		                                               0, 0, 0.0);
		GDateTime * future_bound = g_date_time_new_local(g_date_time_get_year(future),
		                                                 g_date_time_get_month(future),
		                                                 g_date_time_get_day_of_month(future),
		                                                 23, 59, 59.9);
		if (g_date_time_compare(time, past_bound) < 0 ||
		    g_date_time_compare(time, future_bound) > 0) {
			show_date = TRUE;
		}
		g_date_time_unref(past);
		g_date_time_unref(future);
		g_date_time_unref(past_bound);
		g_date_time_unref(future_bound);
	}

	g_date_time_unref (now);

	return generate_format_string_full(show_day, show_date);
}

