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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdk.h>
#include <glib/gi18n.h>
#include "calendar-menu-item.h"

#include "dbus-shared.h"

#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/menuitem.h>

enum {
	LAST_SIGNAL
};

/* static guint signals[LAST_SIGNAL] = { }; */

typedef struct _CalendarMenuItemPrivate CalendarMenuItemPrivate;
struct _CalendarMenuItemPrivate
{
	void * placeholder;
};

#define CALENDAR_MENU_ITEM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CALENDAR_MENU_ITEM_TYPE, CalendarMenuItemPrivate))

/* Prototypes */
static void calendar_menu_item_class_init (CalendarMenuItemClass *klass);
static void calendar_menu_item_init       (CalendarMenuItem *self);
static void calendar_menu_item_dispose    (GObject *object);
static void calendar_menu_item_finalize   (GObject *object);

G_DEFINE_TYPE (CalendarMenuItem, calendar_menu_item, DBUSMENU_TYPE_MENUITEM);

static void
calendar_menu_item_class_init (CalendarMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (CalendarMenuItemPrivate));

	object_class->dispose = calendar_menu_item_dispose;
	object_class->finalize = calendar_menu_item_finalize;

	return;
}

static void
calendar_menu_item_init (CalendarMenuItem *self)
{
	return;
}

static void
calendar_menu_item_dispose (GObject *object)
{
	G_OBJECT_CLASS (calendar_menu_item_parent_class)->dispose (object);
}

static void
calendar_menu_item_finalize (GObject *object)
{
	G_OBJECT_CLASS (calendar_menu_item_parent_class)->finalize (object);

	return;
}

CalendarMenuItem *
calendar_menu_item_new ()
{
	CalendarMenuItem * self = g_object_new(CALENDAR_MENU_ITEM_TYPE, NULL);

	dbusmenu_menuitem_property_set(DBUSMENU_MENUITEM(self), DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CALENDAR_MENUITEM_TYPE);

	return self;
}

