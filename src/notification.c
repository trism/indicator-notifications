/*
 * notification.c - A gobject subclass to represent a org.freedesktop.Notification.Notify message.
 */

#include "notification.h"

#define COLUMN_APP_NAME       0
#define COLUMN_REPLACES_ID    1
#define COLUMN_APP_ICON       2
#define COLUMN_SUMMARY        3
#define COLUMN_BODY           4
#define COLUMN_ACTIONS        5
#define COLUMN_HINTS          6
#define COLUMN_EXPIRE_TIMEOUT 7

#define COLUMN_COUNT 8

#define X_CANONICAL_PRIVATE_SYNCHRONOUS "x-canonical-private-synchronous"

static void notification_class_init(NotificationClass *klass);
static void notification_init(Notification *self);
static void notification_dispose(GObject *object);

G_DEFINE_TYPE (Notification, notification, G_TYPE_OBJECT);

static void
notification_class_init(NotificationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(NotificationPrivate));

  object_class->dispose = notification_dispose;
}

static void
notification_init(Notification *self)
{
  self->priv = NOTIFICATION_GET_PRIVATE(self);

  self->priv->app_name = NULL;
  self->priv->replaces_id = 0;
  self->priv->app_icon = NULL;
  self->priv->summary = NULL;
  self->priv->body = NULL;
  self->priv->expire_timeout = 0;
  self->priv->timestamp = NULL;
  self->priv->is_volume = FALSE;
}

static void
notification_dispose(GObject *object)
{
  Notification *self = NOTIFICATION(object);

  if(self->priv->app_name != NULL) {
    g_free(self->priv->app_name);
    self->priv->app_name = NULL;
  }

  if(self->priv->app_icon != NULL) {
    g_free(self->priv->app_icon);
    self->priv->app_icon = NULL;
  }

  if(self->priv->summary != NULL) {
    g_free(self->priv->summary);
    self->priv->summary = NULL;
  }

  if(self->priv->body != NULL) {
    g_free(self->priv->body);
    self->priv->body = NULL;
  }

  if(self->priv->timestamp != NULL) {
    g_date_time_unref(self->priv->timestamp);
    self->priv->timestamp = NULL;
  }

  G_OBJECT_CLASS(notification_parent_class)->dispose(object);
}

Notification* 
notification_new(void)
{
  return NOTIFICATION(g_object_new(NOTIFICATION_TYPE, NULL));
}

Notification*
notification_new_from_dbus_message(GDBusMessage *message)
{
  Notification *self = notification_new();

  /* timestamp */
  self->priv->timestamp = g_date_time_new_now_local();

  GVariant *body = g_dbus_message_get_body(message);
  GVariant *child = NULL, *value = NULL;
  g_assert(g_variant_is_of_type(body, G_VARIANT_TYPE_TUPLE));
  g_assert(g_variant_n_children(body) == COLUMN_COUNT);

  /* app_name */
  child = g_variant_get_child_value(body, COLUMN_APP_NAME);
  g_assert(g_variant_is_of_type(child, G_VARIANT_TYPE_STRING));
  self->priv->app_name = g_variant_dup_string(child, 
      &(self->priv->app_name_length));

  /* replaces_id */
  child = g_variant_get_child_value(body, COLUMN_REPLACES_ID);
  g_assert(g_variant_is_of_type(child, G_VARIANT_TYPE_UINT32));
  self->priv->replaces_id = g_variant_get_uint32(child);

  /* app_icon */
  child = g_variant_get_child_value(body, COLUMN_APP_ICON);
  g_assert(g_variant_is_of_type(child, G_VARIANT_TYPE_STRING));
  self->priv->app_icon = g_variant_dup_string(child,
      &(self->priv->app_icon_length));

  /* summary */
  child = g_variant_get_child_value(body, COLUMN_SUMMARY);
  g_assert(g_variant_is_of_type(child, G_VARIANT_TYPE_STRING));
  self->priv->summary = g_variant_dup_string(child,
      &(self->priv->summary_length));

  /* body */
  child = g_variant_get_child_value(body, COLUMN_BODY);
  g_assert(g_variant_is_of_type(child, G_VARIANT_TYPE_STRING));
  self->priv->body = g_variant_dup_string(child,
      &(self->priv->body_length));

  /* hints */
  child = g_variant_get_child_value(body, COLUMN_HINTS);
  g_assert(g_variant_is_of_type(child, G_VARIANT_TYPE_DICTIONARY));

  /* check for volume hint */
  value = g_variant_lookup_value(child, X_CANONICAL_PRIVATE_SYNCHRONOUS, G_VARIANT_TYPE_STRING);
  if(value != NULL) {
    if(g_strcmp0(g_variant_get_string(value, NULL), "volume") == 0) {
      self->priv->is_volume = TRUE;
    }
  }

  child = NULL;

  return self;
}

const gchar*
notification_get_app_name(Notification *self)
{
  return self->priv->app_name;
}

const gchar*
notification_get_app_icon(Notification *self)
{
  return self->priv->app_icon;
}

const gchar*
notification_get_summary(Notification *self)
{
  return self->priv->summary;
}

const gchar*
notification_get_body(Notification *self)
{
  return self->priv->body;
}

gint64
notification_get_timestamp(Notification *self)
{
  return g_date_time_to_unix(self->priv->timestamp);
}

gchar*
notification_timestamp_for_locale(Notification *self)
{
  return g_date_time_format(self->priv->timestamp, "%X %x");
}

gboolean
notification_is_volume(Notification *self)
{
  return self->priv->is_volume;
}

void
notification_print(Notification *self)
{
  g_print("app_name = %s\n", self->priv->app_name);
  g_print("app_icon = %s\n", self->priv->app_icon);
  g_print("summary = %s\n", self->priv->summary);
  g_print("body = %s\n", self->priv->body);
}
