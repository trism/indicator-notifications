/*
 * dbus-spy.h - A gobject subclass to watch dbus for org.freedesktop.Notification.Notify messages.
 */

#ifndef __DBUS_SPY_H__
#define __DBUS_SPY_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "notification.h"

G_BEGIN_DECLS

#define DBUS_SPY_TYPE             (dbus_spy_get_type ())
#define DBUS_SPY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUS_SPY_TYPE, DBusSpy))
#define DBUS_SPY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DBUS_SPY_TYPE, DBusSpyClass))
#define IS_DBUS_SPY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUS_SPY_TYPE))
#define IS_DBUS_SPY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUS_SPY_TYPE))

typedef struct _DBusSpy       DBusSpy;
typedef struct _DBusSpyClass  DBusSpyClass;
typedef struct _DBusSpyPrivate DBusSpyPrivate;

struct _DBusSpy
{
  GObject parent;
  DBusSpyPrivate *priv;
};

struct _DBusSpyClass
{
  GObjectClass parent_class;

  void (* message_received) (DBusSpy *spy,
                             Notification *note);
};

struct _DBusSpyPrivate {
  GDBusConnection *connection;
  GCancellable *connection_cancel;
};

#define DBUS_SPY_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DBUS_SPY_TYPE, DBusSpyPrivate))

GType    dbus_spy_get_type(void);
DBusSpy* dbus_spy_new(void);

G_END_DECLS

#endif /* __DBUS_SPY_H__ */
