/*
An example indicator.

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

#ifndef __EXAMPLE_INTERFACE_H__
#define __EXAMPLE_INTERFACE_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define EXAMPLE_INTERFACE_TYPE            (example_interface_get_type ())
#define EXAMPLE_INTERFACE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXAMPLE_INTERFACE_TYPE, ExampleInterface))
#define EXAMPLE_INTERFACE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EXAMPLE_INTERFACE_TYPE, ExampleInterfaceClass))
#define IS_EXAMPLE_INTERFACE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EXAMPLE_INTERFACE_TYPE))
#define IS_EXAMPLE_INTERFACE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EXAMPLE_INTERFACE_TYPE))
#define EXAMPLE_INTERFACE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EXAMPLE_INTERFACE_TYPE, ExampleInterfaceClass))

typedef struct _ExampleInterface        ExampleInterface;
typedef struct _ExampleInterfacePrivate ExampleInterfacePrivate;
typedef struct _ExampleInterfaceClass   ExampleInterfaceClass;

struct _ExampleInterfaceClass {
	GObjectClass parent_class;
};

struct _ExampleInterface {
	GObject parent;
	ExampleInterfacePrivate * priv;
};

GType              example_interface_get_type       (void);

G_END_DECLS

#endif
