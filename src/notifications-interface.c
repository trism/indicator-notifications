/*
An indicator to display recent notifications.

Adapted from: indicator-notifications/src/notifications-service.c by
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>

#include "notifications-interface.h"
#include "gen-notifications-service.xml.h"
#include "dbus-shared.h"

/**
  NotificationsInterfacePrivate:
  @dbus_registration: The handle for this object being registered
  on dbus.

  Structure to define the memory for the private area
  of the notifications interface instance.
*/
struct _NotificationsInterfacePrivate {
  GDBusConnection *bus;
  GCancellable *bus_cancel;
  guint dbus_registration;
};

#define NOTIFICATIONS_INTERFACE_GET_PRIVATE(o) (NOTIFICATIONS_INTERFACE(o)->priv)

/* GDBus Stuff */
static GDBusNodeInfo      *node_info = NULL;
static GDBusInterfaceInfo *interface_info = NULL;

static void notifications_interface_class_init(NotificationsInterfaceClass *klass);
static void notifications_interface_init(NotificationsInterface *self);
static void notifications_interface_dispose(GObject *object);
static void notifications_interface_finalize(GObject *object);
static void bus_get_cb(GObject *object, GAsyncResult *res, gpointer user_data);

G_DEFINE_TYPE (NotificationsInterface, notifications_interface, G_TYPE_OBJECT);

static void
notifications_interface_class_init(NotificationsInterfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private(klass, sizeof(NotificationsInterfacePrivate));

  object_class->dispose = notifications_interface_dispose;
  object_class->finalize = notifications_interface_finalize;

  /* Setting up the DBus interfaces */
  if(node_info == NULL) {
    GError * error = NULL;

    node_info = g_dbus_node_info_new_for_xml(_notifications_service, &error);
    if(error != NULL) {
      g_error("Unable to parse Notifications Service Interface description: %s", error->message);
      g_error_free(error);
    }
  }

  if(interface_info == NULL) {
    interface_info = g_dbus_node_info_lookup_interface(node_info, SERVICE_IFACE);

    if(interface_info == NULL) {
      g_error("Unable to find interface '" SERVICE_IFACE "'");
    }
  }

  return;
}

static void
notifications_interface_init(NotificationsInterface *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, NOTIFICATIONS_INTERFACE_TYPE, NotificationsInterfacePrivate);

  self->priv->bus = NULL;
  self->priv->bus_cancel = NULL;
  self->priv->dbus_registration = 0;

  self->priv->bus_cancel = g_cancellable_new();
  g_bus_get(G_BUS_TYPE_SESSION,
            self->priv->bus_cancel,
            bus_get_cb,
            self);

  return;
}

static void
bus_get_cb(GObject *object, GAsyncResult *res, gpointer user_data)
{
  GError *error = NULL;
  GDBusConnection *connection = g_bus_get_finish(res, &error);

  if(error != NULL) {
    g_error("OMG! Unable to get a connection to DBus: %s", error->message);
    g_error_free(error);
    return;
  }

  NotificationsInterfacePrivate *priv = NOTIFICATIONS_INTERFACE_GET_PRIVATE(user_data);

  g_warn_if_fail(priv->bus == NULL);
  priv->bus = connection;

  if(priv->bus_cancel != NULL) {
    g_object_unref(priv->bus_cancel);
    priv->bus_cancel = NULL;
  }

  /* Now register our object on our new connection */
  priv->dbus_registration = g_dbus_connection_register_object(priv->bus,
                                                              SERVICE_OBJ,
                                                              interface_info,
                                                              NULL,
                                                              user_data,
                                                              NULL,
                                                              &error);

  if(error != NULL) {
    g_error("Unable to register the object to DBus: %s", error->message);
    g_error_free(error);
    return;
  }

  return;	
}

static void
notifications_interface_dispose(GObject *object)
{
  NotificationsInterfacePrivate *priv = NOTIFICATIONS_INTERFACE_GET_PRIVATE(object);

  if(priv->dbus_registration != 0) {
    g_dbus_connection_unregister_object(priv->bus, priv->dbus_registration);
    /* Don't care if it fails, there's nothing we can do */
    priv->dbus_registration = 0;
  }

  if(priv->bus != NULL) {
    g_object_unref(priv->bus);
    priv->bus = NULL;
  }

  if(priv->bus_cancel != NULL) {
    g_cancellable_cancel(priv->bus_cancel);
    g_object_unref(priv->bus_cancel);
    priv->bus_cancel = NULL;
  }

  G_OBJECT_CLASS (notifications_interface_parent_class)->dispose (object);
  return;
}

static void
notifications_interface_finalize(GObject *object)
{
  G_OBJECT_CLASS (notifications_interface_parent_class)->finalize (object);
  return;
}

void
notifications_interface_message_added(NotificationsInterface *self)
{
  g_return_if_fail(IS_NOTIFICATIONS_INTERFACE(self));

  NotificationsInterfacePrivate *priv = NOTIFICATIONS_INTERFACE_GET_PRIVATE(self);
  GError *error = NULL;

  g_dbus_connection_emit_signal(priv->bus,
                                NULL,
                                SERVICE_OBJ,
                                SERVICE_IFACE,
                                "MessageAdded",
                                NULL,
                                &error);

  if(error != NULL) {
    g_error("Unable to send MessageAdded signal: %s", error->message);
    g_error_free(error);
    return;
  }

  return;
}
