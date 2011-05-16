/*
An indicator to display recent notifications.

Adapted from: indicator-datetime/src/indicator-datetime.c by
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

#include <locale.h>
#include <langinfo.h>
#include <string.h>
#include <time.h>

/* GStuff */
#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* Indicator Stuff */
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>

/* DBusMenu */
#include <libdbusmenu-gtk/menu.h>
#include <libido/libido.h>
#include <libdbusmenu-gtk/menuitem.h>

#include "dbus-shared.h"
#include "settings-shared.h"

#define INDICATOR_NOTIFICATIONS_TYPE            (indicator_notifications_get_type ())
#define INDICATOR_NOTIFICATIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_NOTIFICATIONS_TYPE, IndicatorNotifications))
#define INDICATOR_NOTIFICATIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_NOTIFICATIONS_TYPE, IndicatorNotificationsClass))
#define IS_INDICATOR_NOTIFICATIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_NOTIFICATIONS_TYPE))
#define IS_INDICATOR_NOTIFICATIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_NOTIFICATIONS_TYPE))
#define INDICATOR_NOTIFICATIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_NOTIFICATIONS_TYPE, IndicatorNotificationsClass))

typedef struct _IndicatorNotifications         IndicatorNotifications;
typedef struct _IndicatorNotificationsClass    IndicatorNotificationsClass;
typedef struct _IndicatorNotificationsPrivate  IndicatorNotificationsPrivate;

struct _IndicatorNotificationsClass {
  IndicatorObjectClass parent_class;
};

struct _IndicatorNotifications {
  IndicatorObject parent;
  IndicatorNotificationsPrivate *priv;
};

struct _IndicatorNotificationsPrivate {
  GtkImage *image;

  GdkPixbuf *pixbuf_read;
  GdkPixbuf *pixbuf_unread;

  IndicatorServiceManager *sm;
  DbusmenuGtkMenu *menu;

  gchar *accessible_desc;

  GCancellable *service_proxy_cancel;
  GDBusProxy *service_proxy;
};

#define INDICATOR_NOTIFICATIONS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_NOTIFICATIONS_TYPE, IndicatorNotificationsPrivate))

#define INDICATOR_ICON_SIZE 22

GType indicator_notifications_get_type(void);

static void indicator_notifications_class_init(IndicatorNotificationsClass *klass);
static void indicator_notifications_init(IndicatorNotifications *self);
static void indicator_notifications_dispose(GObject *object);
static void indicator_notifications_finalize(GObject *object);
static GtkImage *get_image(IndicatorObject *io);
static GtkMenu *get_menu(IndicatorObject *io);
static const gchar *get_accessible_desc(IndicatorObject *io);
static GdkPixbuf *load_icon(const gchar *name, guint size);
static void menu_visible_notify_cb(GtkWidget *menu, GParamSpec *pspec, gpointer user_data);
static gboolean new_notification_menuitem(DbusmenuMenuitem *new_item, DbusmenuMenuitem *parent, DbusmenuClient *client, gpointer user_data);
static void receive_signal(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, gpointer user_data);
static void service_proxy_cb(GObject *object, GAsyncResult *res, gpointer user_data);

/* Indicator Module Config */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_NOTIFICATIONS_TYPE)

G_DEFINE_TYPE (IndicatorNotifications, indicator_notifications, INDICATOR_OBJECT_TYPE);

static void
indicator_notifications_class_init(IndicatorNotificationsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private(klass, sizeof(IndicatorNotificationsPrivate));

  object_class->dispose = indicator_notifications_dispose;
  object_class->finalize = indicator_notifications_finalize;

  IndicatorObjectClass *io_class = INDICATOR_OBJECT_CLASS(klass);

  io_class->get_image = get_image;
  io_class->get_menu = get_menu;
  io_class->get_accessible_desc = get_accessible_desc;

  return;
}

static void
menu_visible_notify_cb(GtkWidget *menu, G_GNUC_UNUSED GParamSpec *pspec, gpointer user_data)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);

  gboolean visible;
  g_object_get(G_OBJECT(menu), "visible", &visible, NULL);
  if(!visible) {
    if(self->priv->pixbuf_read != NULL) {
      gtk_image_set_from_pixbuf(self->priv->image, self->priv->pixbuf_read);
    }
  }
}

static void
indicator_notifications_init(IndicatorNotifications *self)
{
  self->priv = INDICATOR_NOTIFICATIONS_GET_PRIVATE(self);

  self->priv->service_proxy = NULL;

  self->priv->sm = NULL;
  self->priv->menu = NULL;

  self->priv->image = NULL;
  self->priv->pixbuf_read = NULL;
  self->priv->pixbuf_unread = NULL;

  self->priv->accessible_desc = _("Notifications");

  self->priv->sm = indicator_service_manager_new_version(SERVICE_NAME, SERVICE_VERSION);

  self->priv->menu = dbusmenu_gtkmenu_new(SERVICE_NAME, MENU_OBJ);

  g_signal_connect(self->priv->menu, "notify::visible", G_CALLBACK(menu_visible_notify_cb), self);

  DbusmenuGtkClient *client = dbusmenu_gtkmenu_get_client(self->priv->menu);

  dbusmenu_client_add_type_handler_full(DBUSMENU_CLIENT(client), DBUSMENU_NOTIFICATION_MENUITEM_TYPE, new_notification_menuitem, self, NULL);

  self->priv->service_proxy_cancel = g_cancellable_new();

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
                           G_DBUS_PROXY_FLAGS_NONE,
                           NULL,
                           SERVICE_NAME,
                           SERVICE_OBJ,
                           SERVICE_IFACE,
                           self->priv->service_proxy_cancel,
                           service_proxy_cb,
                           self);

  return;
}

/* Callback from trying to create the proxy for the serivce, this
   could include starting the service.  Sometime it'll fail and
   we'll try to start that dang service again! */
static void
service_proxy_cb(GObject *object, GAsyncResult *res, gpointer user_data)
{
  GError *error = NULL;

  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);
  g_return_if_fail(self != NULL);

  GDBusProxy *proxy = g_dbus_proxy_new_for_bus_finish(res, &error);

  IndicatorNotificationsPrivate *priv = INDICATOR_NOTIFICATIONS_GET_PRIVATE(self);

  if(priv->service_proxy_cancel != NULL) {
    g_object_unref(priv->service_proxy_cancel);
    priv->service_proxy_cancel = NULL;
  }

  if(error != NULL) {
    g_warning("Could not grab DBus proxy for %s: %s", SERVICE_NAME, error->message);
    g_error_free(error);
    return;
  }

  /* Okay, we're good to grab the proxy at this point, we're
  sure that it's ours. */
  priv->service_proxy = proxy;

  g_signal_connect(proxy, "g-signal", G_CALLBACK(receive_signal), self);

  return;
}

static void
indicator_notifications_dispose(GObject *object)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(object);

  if(self->priv->image != NULL) {
    g_object_unref(G_OBJECT(self->priv->image));
    self->priv->image = NULL;
  }

  if(self->priv->pixbuf_read != NULL) {
    g_object_unref(G_OBJECT(self->priv->pixbuf_read));
    self->priv->pixbuf_read = NULL;
  }

  if(self->priv->pixbuf_unread != NULL) {
    g_object_unref(G_OBJECT(self->priv->pixbuf_unread));
    self->priv->pixbuf_unread = NULL;
  }

  if(self->priv->menu != NULL) {
    g_object_unref(G_OBJECT(self->priv->menu));
    self->priv->menu = NULL;
  }

  if(self->priv->sm != NULL) {
    g_object_unref(G_OBJECT(self->priv->sm));
    self->priv->sm = NULL;
  }

  if(self->priv->service_proxy != NULL) {
    g_object_unref(self->priv->service_proxy);
    self->priv->service_proxy = NULL;
  }

  G_OBJECT_CLASS (indicator_notifications_parent_class)->dispose (object);
  return;
}

static void
indicator_notifications_finalize(GObject *object)
{
  /*IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(object);*/

  G_OBJECT_CLASS (indicator_notifications_parent_class)->finalize (object);
  return;
}

/* Receives all signals from the service, routed to the appropriate functions */
static void
receive_signal(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name,
               GVariant *parameters, gpointer user_data)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);

  g_debug("received signal '%s'", signal_name);
  if(g_strcmp0(signal_name, "MessageAdded") == 0) {
    if(self->priv->pixbuf_unread != NULL) {
      gtk_image_set_from_pixbuf(self->priv->image, self->priv->pixbuf_unread);
    }
  }

  return;
}

static gboolean
new_notification_menuitem(DbusmenuMenuitem *new_item, DbusmenuMenuitem *parent, 
                          DbusmenuClient *client, gpointer user_data)
{
  g_debug("New notification item");
  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(new_item), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
  g_return_val_if_fail(IS_INDICATOR_NOTIFICATIONS(user_data), FALSE);

  GtkWidget *item = gtk_menu_item_new();
  gtk_menu_item_set_label(GTK_MENU_ITEM(item), dbusmenu_menuitem_property_get(new_item,
        DBUSMENU_MENUITEM_PROP_LABEL));
  gtk_widget_show(item);

  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), new_item, GTK_MENU_ITEM(item), parent);

  g_object_unref(item);

  return TRUE;
}

static GdkPixbuf *
load_icon(const gchar *name, guint size)
{
  /* TODO:
   * Try to load icon from icon theme before falling back to absolute paths
   */

  GError *error = NULL;

  gchar *path = g_strdup_printf(ICONS_DIR "/%s.svg", name);
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, FALSE, &error);

  if(error != NULL) {
    g_warning("Failed to load icon at '%s': %s", path, error->message);
    pixbuf = NULL;
  }

  g_free(path);

  return pixbuf;
}

static GtkImage *
get_image(IndicatorObject *io)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(io);

  if(self->priv->image == NULL) {
    self->priv->image = GTK_IMAGE(gtk_image_new());

    self->priv->pixbuf_read = load_icon("notification-read", INDICATOR_ICON_SIZE);

    if(self->priv->pixbuf_read == NULL) {
      g_error("Failed to load read icon");
      return NULL;
    }

    self->priv->pixbuf_unread = load_icon("notification-unread", INDICATOR_ICON_SIZE);

    if(self->priv->pixbuf_unread == NULL) {
      g_error("Failed to load unread icon");
      return NULL;
    }

    gtk_image_set_from_pixbuf(self->priv->image, self->priv->pixbuf_read);

    gtk_widget_show(GTK_WIDGET(self->priv->image));
  }

  return self->priv->image;
}

static GtkMenu *
get_menu(IndicatorObject *io)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(io);

  return GTK_MENU(self->priv->menu);
}

static const gchar *
get_accessible_desc(IndicatorObject *io)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(io);

  return self->priv->accessible_desc;
}
