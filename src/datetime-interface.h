/*
An indicator to time and date related information in the menubar.

Copyright 2010 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

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

#ifndef __DATETIME_INTERFACE_H__
#define __DATETIME_INTERFACE_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DATETIME_INTERFACE_TYPE            (datetime_interface_get_type ())
#define DATETIME_INTERFACE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DATETIME_INTERFACE_TYPE, DatetimeInterface))
#define DATETIME_INTERFACE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DATETIME_INTERFACE_TYPE, DatetimeInterfaceClass))
#define IS_DATETIME_INTERFACE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DATETIME_INTERFACE_TYPE))
#define IS_DATETIME_INTERFACE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DATETIME_INTERFACE_TYPE))
#define DATETIME_INTERFACE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DATETIME_INTERFACE_TYPE, DatetimeInterfaceClass))

typedef struct _DatetimeInterface        DatetimeInterface;
typedef struct _DatetimeInterfacePrivate DatetimeInterfacePrivate;
typedef struct _DatetimeInterfaceClass   DatetimeInterfaceClass;

struct _DatetimeInterfaceClass {
	GObjectClass parent_class;

	void (*update_time) (void);
};

struct _DatetimeInterface {
	GObject parent;
	DatetimeInterfacePrivate * priv;
};

GType              datetime_interface_get_type       (void);
void               datetime_interface_update         (DatetimeInterface *self);

G_END_DECLS

#endif
