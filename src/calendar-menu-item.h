/*
Calendar menu item dbusmenu "transport" for the corresponding IDO widget.

Copyright 2010 Canonical Ltd.

Authors:
    David Barth <david.barth@canonical.com>

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

#ifndef __CALENDAR_MENU_ITEM_H__
#define __CALENDAR_MENU_ITEM_H__

#include <glib.h>
#include <glib-object.h>

#include <libdbusmenu-glib/menuitem.h>

G_BEGIN_DECLS

#define CALENDAR_MENU_ITEM_TYPE            (calendar_menu_item_get_type ())
#define CALENDAR_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CALENDAR_MENU_ITEM_TYPE, CalendarMenuItem))
#define CALENDAR_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CALENDAR_MENU_ITEM_TYPE, CalendarMenuItemClass))
#define IS_CALENDAR_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CALENDAR_MENU_ITEM_TYPE))
#define IS_CALENDAR_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CALENDAR_MENU_ITEM_TYPE))
#define CALENDAR_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CALENDAR_MENU_ITEM_TYPE, CalendarMenuItemClass))

#define CALENDAR_MENU_ITEM_SIGNAL_ACTIVATE   "activate"
#define CALENDAR_MENUITEM_PROP_TEXT          "text"

typedef struct _CalendarMenuItem      CalendarMenuItem;
typedef struct _CalendarMenuItemClass CalendarMenuItemClass;

struct _CalendarMenuItemClass {
	DbusmenuMenuitemClass parent_class;
};

struct _CalendarMenuItem {
	DbusmenuMenuitem parent;
};

GType calendar_menu_item_get_type (void);
CalendarMenuItem * calendar_menu_item_new ();

G_END_DECLS

#endif /* __CALENDAR_MENU_ITEM_H__ */

