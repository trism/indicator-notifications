/*
 * dbus-spy.c - A gobject subclass to watch dbus for org.freedesktop.Notification.Notify messages.
 */

#include "dbus-spy.h"

enum {
  MESSAGE_RECEIVED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void dbus_spy_class_init(DBusSpyClass *klass);
static void dbus_spy_init(DBusSpy *self);
static void dbus_spy_dispose(GObject *object);

static void add_filter(DBusSpy *self);

static void bus_get_cb(GObject *source_object, GAsyncResult *res, gpointer user_data);

static GDBusMessage *message_filter(GDBusConnection *connection, GDBusMessage *message, 
                                    gboolean incoming, gpointer user_data);

#define MATCH_STRING "type='method_call',interface='org.freedesktop.Notifications',member='Notify'"

G_DEFINE_TYPE (DBusSpy, dbus_spy, G_TYPE_OBJECT);

static void
dbus_spy_class_init(DBusSpyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(DBusSpyPrivate));

  object_class->dispose = dbus_spy_dispose;

  signals[MESSAGE_RECEIVED] =
    g_signal_new(DBUS_SPY_SIGNAL_MESSAGE_RECEIVED,
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(DBusSpyClass, message_received),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE,
                 1, NOTIFICATION_TYPE);
}

static void
bus_get_cb(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GError *error = NULL;

  GDBusConnection *connection = g_bus_get_finish(res, &error);

  if(error != NULL) {
    g_warning("Could not get a connection to the dbus session bus: %s\n", error->message);
    g_error_free(error);
    return;
  }

  DBusSpy *self = DBUS_SPY(user_data);
  g_return_if_fail(self != NULL);

  if(self->priv->connection_cancel != NULL) {
    g_object_unref(self->priv->connection_cancel);
    self->priv->connection_cancel = NULL;
  }

  self->priv->connection = connection;

  add_filter(self);
}

static void
add_filter(DBusSpy *self)
{
  GDBusMessage *message;
  GVariant *body;
  GError *error = NULL;

  message = g_dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
      "org.freedesktop.DBus", "AddMatch");

  body = g_variant_new_parsed("(%s,)", MATCH_STRING);

  g_dbus_message_set_body(message, body);
  
  g_dbus_connection_send_message(self->priv->connection, 
                                 message, 
                                 G_DBUS_SEND_MESSAGE_FLAGS_NONE, 
                                 NULL, 
                                 &error);
  if(error != NULL) {
    g_warning("Failed to send AddMatch message: %s\n", error->message);
    g_error_free(error);
    return;
  }

  g_dbus_connection_add_filter(self->priv->connection, message_filter, self, NULL);
}

static GDBusMessage* 
message_filter(GDBusConnection *connection, GDBusMessage *message, gboolean incoming, gpointer user_data)
{
  if(!incoming) return message;

  GDBusMessageType type = g_dbus_message_get_message_type(message);
  const gchar *interface = g_dbus_message_get_interface(message);
  const gchar *member = g_dbus_message_get_member(message);

  if((type == G_DBUS_MESSAGE_TYPE_METHOD_CALL)
      && (g_strcmp0(interface, "org.freedesktop.Notifications") == 0)
      && (g_strcmp0(member, "Notify") == 0))
  {
    DBusSpy *spy = DBUS_SPY(user_data);
    Notification *note = notification_new_from_dbus_message(message);
    g_signal_emit(spy, signals[MESSAGE_RECEIVED], 0, note);
    g_object_unref(note);
    g_object_unref(message);
    message = NULL;
  }

  return message;
}

static void
dbus_spy_init(DBusSpy *self)
{
  self->priv = DBUS_SPY_GET_PRIVATE(self);

  self->priv->connection = NULL;
  self->priv->connection_cancel = g_cancellable_new();

  g_bus_get(G_BUS_TYPE_SESSION,
            self->priv->connection_cancel,
            bus_get_cb,
            self);
}

static void
dbus_spy_dispose(GObject *object)
{
  DBusSpy *self = DBUS_SPY(object);

  if(self->priv->connection_cancel != NULL) {
    g_cancellable_cancel(self->priv->connection_cancel);
    g_object_unref(self->priv->connection_cancel);
    self->priv->connection_cancel = NULL;
  }

  if(self->priv->connection != NULL) {
    g_dbus_connection_close(self->priv->connection, NULL, NULL, NULL);
    g_object_unref(self->priv->connection);
    self->priv->connection = NULL;
  }

  G_OBJECT_CLASS(dbus_spy_parent_class)->dispose(object);
}

DBusSpy* 
dbus_spy_new(void)
{
  return DBUS_SPY(g_object_new(DBUS_SPY_TYPE, NULL));
}

