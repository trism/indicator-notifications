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

#ifndef __DATETIME_UTILS_H__
#define __DATETIME_UTILS_H__

#include <glib.h>

G_BEGIN_DECLS

gboolean is_locale_12h (void);
void split_settings_location (const gchar * location, gchar ** zone, gchar ** name);
gchar * get_current_zone_name (const gchar * location);
gchar * generate_format_string_full (gboolean show_day, gboolean show_date);
gchar * generate_format_string_at_time (GDateTime * time);

G_END_DECLS

#endif
