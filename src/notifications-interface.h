/*
An indicator to display recent notifications.

Adapted from: indicator-notifications/src/notifications-interface.c by
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

#ifndef __NOTIFICATIONS_INTERFACE_H__
#define __NOTIFICATIONS_INTERFACE_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define NOTIFICATIONS_INTERFACE_TYPE            (notifications_interface_get_type ())
#define NOTIFICATIONS_INTERFACE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NOTIFICATIONS_INTERFACE_TYPE, NotificationsInterface))
#define NOTIFICATIONS_INTERFACE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NOTIFICATIONS_INTERFACE_TYPE, NotificationsInterfaceClass))
#define IS_NOTIFICATIONS_INTERFACE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NOTIFICATIONS_INTERFACE_TYPE))
#define IS_NOTIFICATIONS_INTERFACE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NOTIFICATIONS_INTERFACE_TYPE))
#define NOTIFICATIONS_INTERFACE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NOTIFICATIONS_INTERFACE_TYPE, NotificationsInterfaceClass))

typedef struct _NotificationsInterface        NotificationsInterface;
typedef struct _NotificationsInterfacePrivate NotificationsInterfacePrivate;
typedef struct _NotificationsInterfaceClass   NotificationsInterfaceClass;

struct _NotificationsInterfaceClass {
  GObjectClass parent_class;

  void (*message_added) (void);
};

struct _NotificationsInterface {
  GObject parent;
  NotificationsInterfacePrivate *priv;
};

GType                   notifications_interface_get_type(void);
NotificationsInterface *notifications_interface_new();
void                    notifications_interface_message_added(NotificationsInterface *self);

G_END_DECLS

#endif
