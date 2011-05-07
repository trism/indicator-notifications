/* -*- Mode: C; coding: utf-8; indent-tabs-mode: nil; tab-width: 2 -*-

Copyright 2011 Canonical Ltd.

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

#ifndef __TIMEZONE_COMPLETION_H__
#define __TIMEZONE_COMPLETION_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TIMEZONE_COMPLETION_TYPE            (timezone_completion_get_type ())
#define TIMEZONE_COMPLETION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TIMEZONE_COMPLETION_TYPE, TimezoneCompletion))
#define TIMEZONE_COMPLETION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TIMEZONE_COMPLETION_TYPE, TimezoneCompletionClass))
#define IS_TIMEZONE_COMPLETION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TIMEZONE_COMPLETION_TYPE))
#define IS_TIMEZONE_COMPLETION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TIMEZONE_COMPLETION_TYPE))
#define TIMEZONE_COMPLETION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TIMEZONE_COMPLETION_TYPE, TimezoneCompletionClass))

typedef struct _TimezoneCompletion      TimezoneCompletion;
typedef struct _TimezoneCompletionClass TimezoneCompletionClass;

struct _TimezoneCompletionClass {
  GtkEntryCompletionClass parent_class;
};

struct _TimezoneCompletion {
  GtkEntryCompletion parent;
};

#define TIMEZONE_COMPLETION_ZONE      0
#define TIMEZONE_COMPLETION_NAME      1
#define TIMEZONE_COMPLETION_ADMIN1    2
#define TIMEZONE_COMPLETION_COUNTRY   3
#define TIMEZONE_COMPLETION_LONGITUDE 4
#define TIMEZONE_COMPLETION_LATITUDE  5
#define TIMEZONE_COMPLETION_LAST      6

GType timezone_completion_get_type (void);
TimezoneCompletion * timezone_completion_new ();
void timezone_completion_watch_entry (TimezoneCompletion * completion, GtkEntry * entry);

G_END_DECLS

#endif /* __TIMEZONE_COMPLETION_H__ */

