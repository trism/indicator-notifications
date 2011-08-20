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
#if WITH_GTK == 3
#include <libdbusmenu-gtk3/menu.h>
#include <libdbusmenu-gtk3/menuitem.h>
#else
#include <libdbusmenu-gtk/menu.h>
#include <libdbusmenu-gtk/menuitem.h>
#endif

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

  gboolean have_unread;

  IndicatorServiceManager *sm;
  DbusmenuGtkMenu *menu;

  gchar *accessible_desc;

  GCancellable *service_proxy_cancel;
  GDBusProxy *service_proxy;
};

#define INDICATOR_NOTIFICATIONS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_NOTIFICATIONS_TYPE, IndicatorNotificationsPrivate))

#define INDICATOR_ICON_SIZE 22
#define INDICATOR_ICON_READ   "indicator-notification-read"
#define INDICATOR_ICON_UNREAD "indicator-notification-unread"

GType indicator_notifications_get_type(void);

static void indicator_notifications_class_init(IndicatorNotificationsClass *klass);
static void indicator_notifications_init(IndicatorNotifications *self);
static void indicator_notifications_dispose(GObject *object);
static void indicator_notifications_finalize(GObject *object);
static GtkImage *get_image(IndicatorObject *io);
static GtkMenu *get_menu(IndicatorObject *io);
static const gchar *get_accessible_desc(IndicatorObject *io);
static GdkPixbuf *load_icon(const gchar *name, gint size);
static void menu_visible_notify_cb(GtkWidget *menu, GParamSpec *pspec, gpointer user_data);
static gboolean new_notification_menuitem(DbusmenuMenuitem *new_item, DbusmenuMenuitem *parent, DbusmenuClient *client, gpointer user_data);
static void receive_signal(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name, GVariant *parameters, gpointer user_data);
static void service_proxy_cb(GObject *object, GAsyncResult *res, gpointer user_data);
static void style_changed(GtkWidget *widget, GtkStyle *oldstyle, gpointer user_data);

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
  if(visible) {
    gtk_menu_reposition(GTK_MENU(menu));
  }
  else {
    if(self->priv->pixbuf_read != NULL) {
      self->priv->have_unread = FALSE;
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

  self->priv->have_unread = FALSE;

  self->priv->accessible_desc = _("Notifications");

  self->priv->sm = indicator_service_manager_new_version(SERVICE_NAME, SERVICE_VERSION);

  self->priv->menu = dbusmenu_gtkmenu_new(SERVICE_NAME, MENU_OBJ);

  g_signal_connect(self->priv->menu, "notify::visible", G_CALLBACK(menu_visible_notify_cb), self);

  DbusmenuGtkClient *client = dbusmenu_gtkmenu_get_client(self->priv->menu);

  dbusmenu_client_add_type_handler_full(DBUSMENU_CLIENT(client), 
      NOTIFICATION_MENUITEM_TYPE, 
      new_notification_menuitem, 
      self, NULL);

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

static void
receive_signal(GDBusProxy *proxy, gchar *sender_name, gchar *signal_name,
               GVariant *parameters, gpointer user_data)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);

  g_debug("received signal '%s'", signal_name);
  if(g_strcmp0(signal_name, "MessageAdded") == 0) {
    if(self->priv->pixbuf_unread != NULL) {
      self->priv->have_unread = TRUE;
      gtk_image_set_from_pixbuf(self->priv->image, self->priv->pixbuf_unread);
    }
  }

  return;
}

static gboolean
new_notification_menuitem(DbusmenuMenuitem *new_item, DbusmenuMenuitem *parent, 
                          DbusmenuClient *client, gpointer user_data)
{
  g_return_val_if_fail(DBUSMENU_IS_MENUITEM(new_item), FALSE);
  g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
  g_return_val_if_fail(IS_INDICATOR_NOTIFICATIONS(user_data), FALSE);

  gchar *app_name = g_markup_escape_text(dbusmenu_menuitem_property_get(new_item,
        NOTIFICATION_MENUITEM_PROP_APP_NAME), -1);
  gchar *summary = g_markup_escape_text(dbusmenu_menuitem_property_get(new_item,
        NOTIFICATION_MENUITEM_PROP_SUMMARY), -1);
  gchar *body = g_markup_escape_text(dbusmenu_menuitem_property_get(new_item,
        NOTIFICATION_MENUITEM_PROP_BODY), -1);
  gchar *timestamp_string = g_markup_escape_text(dbusmenu_menuitem_property_get(new_item,
        NOTIFICATION_MENUITEM_PROP_TIMESTAMP_STRING), -1);

  gchar *markup = g_strdup_printf("<b>%s</b>\n%s\n<small><i>%s %s <b>%s</b></i></small>",
      summary, body, timestamp_string, _("from"), app_name);

  g_free(app_name);
  g_free(summary);
  g_free(body);
  g_free(timestamp_string);

#if WITH_GTK == 3
  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
#endif

  GtkWidget *label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
  gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
  gtk_label_set_markup(GTK_LABEL(label), markup);
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
  gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);

#if WITH_GTK == 3
  gtk_label_set_max_width_chars(GTK_LABEL(label), 42);
#else
  gtk_widget_set_size_request(label, 300, -1);
#endif

  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  gtk_widget_show(label);

  g_free(markup);

  GtkWidget *item = gtk_menu_item_new();
  gtk_container_add(GTK_CONTAINER(item), hbox);
  gtk_widget_show(hbox);

  dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), new_item, GTK_MENU_ITEM(item), parent);

  return TRUE;
}

static void 
style_changed(GtkWidget *widget, GtkStyle *oldstyle, gpointer user_data)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);
  GdkPixbuf *pixbuf_read = NULL, *pixbuf_unread = NULL;

  /* Attempt to load the new pixbufs, but keep the ones we have if this fails */
  pixbuf_read = load_icon(INDICATOR_ICON_READ, INDICATOR_ICON_SIZE);
  if(pixbuf_read != NULL) {
    g_object_unref(self->priv->pixbuf_read);
    self->priv->pixbuf_read = pixbuf_read;

    if(!self->priv->have_unread) {
      gtk_image_set_from_pixbuf(self->priv->image, pixbuf_read);
    }
  }
  else {
    g_warning("Failed to update read icon to new theme");
  }

  pixbuf_unread = load_icon(INDICATOR_ICON_UNREAD, INDICATOR_ICON_SIZE);
  if(pixbuf_unread != NULL) {
    g_object_unref(self->priv->pixbuf_unread);
    self->priv->pixbuf_unread = pixbuf_unread;

    if(self->priv->have_unread) {
      gtk_image_set_from_pixbuf(self->priv->image, pixbuf_unread);
    }
  }
  else {
    g_warning("Failed to update unread icon to new theme");
  }
}

static GdkPixbuf *
load_icon(const gchar *name, gint size)
{
  GError *error = NULL;
  GdkPixbuf *pixbuf = NULL;

  /* First try to load the icon from the icon theme */
  GtkIconTheme *theme = gtk_icon_theme_get_default();

  if(gtk_icon_theme_has_icon(theme, name)) {
    pixbuf = gtk_icon_theme_load_icon(theme, name, size, GTK_ICON_LOOKUP_FORCE_SVG, &error);

    if(error != NULL) {
      g_warning("Failed to load icon '%s' from icon theme: %s", name, error->message);
    }
    else {
      return pixbuf;
    }
  }

  /* Otherwise load from the icon installation path */
  gchar *path = g_strdup_printf(ICONS_DIR "/hicolor/scalable/status/%s.svg", name);
  pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, FALSE, &error);

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

    self->priv->pixbuf_read = load_icon(INDICATOR_ICON_READ, INDICATOR_ICON_SIZE);

    if(self->priv->pixbuf_read == NULL) {
      g_error("Failed to load read icon");
      return NULL;
    }

    self->priv->pixbuf_unread = load_icon(INDICATOR_ICON_UNREAD, INDICATOR_ICON_SIZE);

    if(self->priv->pixbuf_unread == NULL) {
      g_error("Failed to load unread icon");
      return NULL;
    }

    gtk_image_set_from_pixbuf(self->priv->image, self->priv->pixbuf_read);

    g_signal_connect(G_OBJECT(self->priv->image), "style-set", G_CALLBACK(style_changed), self);

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
