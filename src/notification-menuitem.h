/*
 * notification-menuitem.h - A menuitem to display notifications.
 */

#ifndef __NOTIFICATION_MENUITEM_H__
#define __NOTIFICATION_MENUITEM_H__

#include <gtk/gtk.h>
#include "notification.h"

G_BEGIN_DECLS

#define NOTIFICATION_MENUITEM_TYPE             (notification_menuitem_get_type ())
#define NOTIFICATION_MENUITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), NOTIFICATION_MENUITEM_TYPE, NotificationMenuItem))
#define NOTIFICATION_MENUITEM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), NOTIFICATION_MENUITEM_TYPE, NotificationMenuItemClass))
#define IS_NOTIFICATION_MENUITEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NOTIFICATION_MENUITEM_TYPE))
#define IS_NOTIFICATION_MENUITEM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), NOTIFICATION_MENUITEM_TYPE))

typedef struct _NotificationMenuItem        NotificationMenuItem;
typedef struct _NotificationMenuItemClass   NotificationMenuItemClass;
typedef struct _NotificationMenuItemPrivate NotificationMenuItemPrivate;

struct _NotificationMenuItem
{
  GtkMenuItem parent_instance;
  NotificationMenuItemPrivate *priv;
};

struct _NotificationMenuItemClass
{
  GtkMenuItemClass parent_class;
};

struct _NotificationMenuItemPrivate {
  GtkWidget *label;
  GtkWidget *close_image;
};

#define NOTIFICATION_MENUITEM_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NOTIFICATION_MENUITEM_TYPE, NotificationMenuItemPrivate))

GType      notification_menuitem_get_type(void);
GtkWidget *notification_menuitem_new(void);
void       notification_menuitem_set_from_notification(NotificationMenuItem *self, Notification *note);

G_END_DECLS

#endif /* __NOTIFICATION_MENUITEM_H__ */
