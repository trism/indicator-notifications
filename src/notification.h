/*
 * notification.h - A gobject subclass to represent a org.freedesktop.Notification.Notify message.
 */

#ifndef __NOTIFICATION_H__
#define __NOTIFICATION_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <time.h>

G_BEGIN_DECLS

#define NOTIFICATION_TYPE             (notification_get_type ())
#define NOTIFICATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), NOTIFICATION_TYPE, Notification))
#define NOTIFICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), NOTIFICATION_TYPE, NotificationClass))
#define IS_NOTIFICATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NOTIFICATION_TYPE))
#define IS_NOTIFICATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), NOTIFICATION_TYPE))

typedef struct _Notification        Notification;
typedef struct _NotificationClass   NotificationClass;
typedef struct _NotificationPrivate NotificationPrivate;

struct _Notification
{
  GObject              parent;
  NotificationPrivate *priv;
};

struct _NotificationClass
{
  GObjectClass parent_class;
};

struct _NotificationPrivate {
  gchar     *app_name;
  gsize      app_name_length;
  guint32    replaces_id;
  gchar     *app_icon;
  gsize      app_icon_length;
  gchar     *summary;
  gsize      summary_length;
  gchar     *body;
  gsize      body_length;
  gint       expire_timeout;
  GDateTime *timestamp;

  gboolean   is_volume;
};

#define NOTIFICATION_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NOTIFICATION_TYPE, NotificationPrivate))

GType         notification_get_type(void);
Notification *notification_new(void);
Notification *notification_new_from_dbus_message(GDBusMessage *);
const gchar  *notification_get_app_name(Notification *);
const gchar  *notification_get_app_icon(Notification *);
const gchar  *notification_get_summary(Notification *);
const gchar  *notification_get_body(Notification *);
gint64        notification_get_timestamp(Notification *);
gchar        *notification_timestamp_for_locale(Notification *);
gboolean      notification_is_volume(Notification *);
void          notification_print(Notification *);

G_END_DECLS

#endif /* __NOTIFICATION_H__ */
