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
#include <glib/gi18n-lib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* Indicator Stuff */
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>

#include "dbus-spy.h"
#include "notification-menuitem.h"

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
  GtkImage    *image;

  GList       *visible_items;
  GList       *hidden_items;

  gboolean     have_unread;
  gboolean     hide_indicator;

  gint         max_items;

  GtkMenu     *menu;
  GtkWidget   *clear_item;
  GtkWidget   *clear_item_label;

  gchar       *accessible_desc;

  DBusSpy     *spy;

  GHashTable  *blacklist;

  GSettings   *settings;
};

#define INDICATOR_NOTIFICATIONS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_NOTIFICATIONS_TYPE, IndicatorNotificationsPrivate))

#define NOTIFICATIONS_SCHEMA             "net.launchpad.indicator.notifications"
#define NOTIFICATIONS_KEY_BLACKLIST      "blacklist"
#define NOTIFICATIONS_KEY_HIDE_INDICATOR "hide-indicator"
#define NOTIFICATIONS_KEY_MAX_ITEMS      "max-items"

#define INDICATOR_ICON_SIZE 22
#define INDICATOR_ICON_READ   "indicator-notification-read"
#define INDICATOR_ICON_UNREAD "indicator-notification-unread"

GType indicator_notifications_get_type(void);

/* Indicator Class Functions */
static void indicator_notifications_class_init(IndicatorNotificationsClass *klass);
static void indicator_notifications_init(IndicatorNotifications *self);
static void indicator_notifications_dispose(GObject *object);
static void indicator_notifications_finalize(GObject *object);

/* Indicator Standard Methods */
static GtkImage    *get_image(IndicatorObject *io);
static GtkMenu     *get_menu(IndicatorObject *io);
static const gchar *get_accessible_desc(IndicatorObject *io);
static void         indicator_notifications_middle_click(IndicatorObject *io, 
                                                         IndicatorObjectEntry *entry,
                                                         guint time,
                                                         gpointer user_data);

/* Utility Functions */
static void       clear_menuitems(IndicatorNotifications *self);
static void       insert_menuitem(IndicatorNotifications *self, GtkWidget *item);
static void       remove_menuitem(IndicatorNotifications *self, GtkWidget *item);
static void       set_unread(IndicatorNotifications *self, gboolean unread);
static void       update_blacklist(IndicatorNotifications *self);
static void       update_clear_item_markup(IndicatorNotifications *self);
static void       update_indicator_visibility(IndicatorNotifications *self);

/* Callbacks */
static void clear_item_activated_cb(GtkMenuItem *menuitem, gpointer user_data);
static void menu_visible_notify_cb(GtkWidget *menu, GParamSpec *pspec, gpointer user_data);
static void message_received_cb(DBusSpy *spy, Notification *note, gpointer user_data);
static void notification_clicked_cb(NotificationMenuItem *menuitem, guint button, gpointer user_data);
static void setting_changed_cb(GSettings *settings, gchar *key, gpointer user_data);

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
  io_class->secondary_activate = indicator_notifications_middle_click;

  return;
}

static void
indicator_notifications_init(IndicatorNotifications *self)
{
  self->priv = INDICATOR_NOTIFICATIONS_GET_PRIVATE(self);

  self->priv->menu = NULL;

  self->priv->image = NULL;

  self->priv->have_unread = FALSE;

  self->priv->accessible_desc = _("Notifications");

  self->priv->visible_items = NULL;
  self->priv->hidden_items = NULL;

  self->priv->menu = GTK_MENU(gtk_menu_new());
  g_signal_connect(self->priv->menu, "notify::visible", G_CALLBACK(menu_visible_notify_cb), self);

  /* Create the clear menuitem */
  self->priv->clear_item_label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(self->priv->clear_item_label), 0, 0);
  gtk_label_set_use_markup(GTK_LABEL(self->priv->clear_item_label), TRUE);
  update_clear_item_markup(self);
  gtk_widget_show(self->priv->clear_item_label);

  self->priv->clear_item = gtk_menu_item_new();
  g_signal_connect(self->priv->clear_item, "activate", G_CALLBACK(clear_item_activated_cb), self);
  gtk_container_add(GTK_CONTAINER(self->priv->clear_item), self->priv->clear_item_label);
  gtk_widget_show(self->priv->clear_item);

  gtk_menu_shell_prepend(GTK_MENU_SHELL(self->priv->menu), self->priv->clear_item);

  /* Watch for notifications from dbus */
  self->priv->spy = dbus_spy_new();
  g_signal_connect(self->priv->spy, DBUS_SPY_SIGNAL_MESSAGE_RECEIVED, G_CALLBACK(message_received_cb), self);

  /* Initialize an empty blacklist */
  self->priv->blacklist = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

  /* Connect to GSettings */
  self->priv->settings = g_settings_new(NOTIFICATIONS_SCHEMA);
  self->priv->hide_indicator = g_settings_get_boolean(self->priv->settings, NOTIFICATIONS_KEY_HIDE_INDICATOR);
  self->priv->max_items = g_settings_get_int(self->priv->settings, NOTIFICATIONS_KEY_MAX_ITEMS);
  update_blacklist(self);
  g_signal_connect(self->priv->settings, "changed", G_CALLBACK(setting_changed_cb), self);
}

static void
indicator_notifications_dispose(GObject *object)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(object);

  if(self->priv->image != NULL) {
    g_object_unref(G_OBJECT(self->priv->image));
    self->priv->image = NULL;
  }

  if(self->priv->visible_items != NULL) {
    g_list_free_full(self->priv->visible_items, g_object_unref);
    self->priv->visible_items = NULL;
  }

  if(self->priv->hidden_items != NULL) {
    g_list_free_full(self->priv->hidden_items, g_object_unref);
    self->priv->hidden_items = NULL;
  }

  if(self->priv->menu != NULL) {
    g_object_unref(G_OBJECT(self->priv->menu));
    self->priv->menu = NULL;
  }

  if(self->priv->spy != NULL) {
    g_object_unref(G_OBJECT(self->priv->spy));
    self->priv->spy = NULL;
  }

  if(self->priv->settings != NULL) {
    g_object_unref(G_OBJECT(self->priv->settings));
    self->priv->settings = NULL;
  }

  if(self->priv->blacklist != NULL) {
    g_hash_table_unref(self->priv->blacklist);
    self->priv->blacklist = NULL;
  }

  G_OBJECT_CLASS (indicator_notifications_parent_class)->dispose (object);
  return;
}

static void
indicator_notifications_finalize(GObject *object)
{
  G_OBJECT_CLASS (indicator_notifications_parent_class)->finalize (object);
  return;
}

static GtkImage *
get_image(IndicatorObject *io)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(io);

  if(self->priv->image == NULL) {
    self->priv->image = GTK_IMAGE(gtk_image_new());
    set_unread(self, FALSE);
    update_indicator_visibility(self);
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

static void
indicator_notifications_middle_click(IndicatorObject *io, IndicatorObjectEntry *entry,
                                     guint time, gpointer user_data)
{
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(io);

  if(g_list_length(self->priv->visible_items) > 0)
    set_unread(self, !self->priv->have_unread);
}

/**
 * clear_menuitems:
 * @self: the indicator
 *
 * Clear all notification menuitems from the menu and the visible/hidden lists.
 **/
static void
clear_menuitems(IndicatorNotifications *self)
{
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(self));
  GList *item;

  /* Remove each visible item from the menu */
  for(item = self->priv->visible_items; item; item = item->next) {
    gtk_container_remove(GTK_CONTAINER(self->priv->menu), GTK_WIDGET(item->data));
  }

  /* Clear the lists */
  g_list_free_full(self->priv->visible_items, g_object_unref);
  self->priv->visible_items = NULL;

  g_list_free_full(self->priv->hidden_items, g_object_unref);
  self->priv->hidden_items = NULL;

  update_clear_item_markup(self);
}

/**
 * insert_menuitem:
 * @self: the indicator
 * @item: the menuitem to insert
 *
 * Inserts a menuitem into the indicator's menu and updates the visible and
 * hidden lists.
 **/
static void
insert_menuitem(IndicatorNotifications *self, GtkWidget *item)
{
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(self));
  g_return_if_fail(GTK_IS_MENU_ITEM(item));
  GList     *last_item;
  GtkWidget *last_widget;

  /* List holds a ref to the menuitem */
  self->priv->visible_items = g_list_prepend(self->priv->visible_items, g_object_ref(item));
  gtk_menu_shell_prepend(GTK_MENU_SHELL(self->priv->menu), item);

  /* Move items that overflow to the hidden list */
  while(g_list_length(self->priv->visible_items) > self->priv->max_items) {
    last_item = g_list_last(self->priv->visible_items);  
    last_widget = GTK_WIDGET(last_item->data);
    /* Steal the ref from the visible list */
    self->priv->visible_items = g_list_delete_link(self->priv->visible_items, last_item);
    self->priv->hidden_items = g_list_prepend(self->priv->hidden_items, last_widget);
    gtk_container_remove(GTK_CONTAINER(self->priv->menu), last_widget);
    last_item = NULL;
    last_widget = NULL;
  }

  update_clear_item_markup(self);
}

/**
 * remove_menuitem:
 * @self: the indicator object
 * @item: the menuitem
 *
 * Removes a menuitem from the indicator menu and the visible list.
 **/
static void
remove_menuitem(IndicatorNotifications *self, GtkWidget *item)
{
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(self));
  g_return_if_fail(GTK_IS_MENU_ITEM(item));

  GList *list_item = g_list_find(self->priv->visible_items, item);

  if(list_item == NULL) {
    g_warning("Attempt to remove menuitem not in visible list");
    return;
  }

  /* Remove the item */
  gtk_container_remove(GTK_CONTAINER(self->priv->menu), item);
  self->priv->visible_items = g_list_delete_link(self->priv->visible_items, list_item);
  g_object_unref(item);

  /* Add an item from the hidden list, if available */
  if(g_list_length(self->priv->hidden_items) > 0) {
    list_item = g_list_first(self->priv->hidden_items);
    GtkWidget *list_widget = GTK_WIDGET(list_item->data);
    self->priv->hidden_items = g_list_delete_link(self->priv->hidden_items, list_item);
    gtk_menu_shell_insert(GTK_MENU_SHELL(self->priv->menu), list_widget,
        g_list_length(self->priv->visible_items));
    /* Steal the ref back from the hidden list */
    self->priv->visible_items = g_list_append(self->priv->visible_items, list_widget);
  }

  update_clear_item_markup(self);
}

/**
 * set_unread:
 * @self: the indicator object
 * @unread: the unread status
 *
 * Sets the unread status of the indicator and updates the icons.
 **/
static void
set_unread(IndicatorNotifications *self, gboolean unread)
{
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(self));

  if(unread) {
    gtk_image_set_from_icon_name(self->priv->image, INDICATOR_ICON_UNREAD, GTK_ICON_SIZE_MENU);
  }
  else {
    gtk_image_set_from_icon_name(self->priv->image, INDICATOR_ICON_READ, GTK_ICON_SIZE_MENU);
  }

  self->priv->have_unread = unread;
}

/**
 * update_blacklist:
 * @self: the indicator object
 *
 * Updates the blacklist from GSettings. This currently does not filter already
 * allowed messages. It only applies to messages received in the future.
 **/
static void
update_blacklist(IndicatorNotifications *self)
{
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(self));
  g_return_if_fail(self->priv->blacklist != NULL);

  g_hash_table_remove_all(self->priv->blacklist);
  gchar **items = g_settings_get_strv(self->priv->settings, NOTIFICATIONS_KEY_BLACKLIST);
  int i;

  for(i = 0; items[i] != NULL; i++) {
    g_hash_table_insert(self->priv->blacklist, g_strdup(items[i]), NULL);
  }

  g_strfreev(items);
}

/**
 * update_clear_item_markup:
 * @self: the indicator object
 *
 * Updates the clear menuitem's label markup based on the number of
 * notifications available.
 **/
static void
update_clear_item_markup(IndicatorNotifications *self)
{
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(self));
  guint visible_length = g_list_length(self->priv->visible_items);
  guint hidden_length = g_list_length(self->priv->hidden_items);
  guint total_length = visible_length + hidden_length;

  gchar *markup = g_strdup_printf(ngettext(
        "Clear <small>(%d Notification)</small>",
        "Clear <small>(%d Notifications)</small>",
        total_length),
      total_length);

  gtk_label_set_markup(GTK_LABEL(self->priv->clear_item_label), markup);
  g_free(markup);

  if (total_length == 0) {
    gtk_menu_shell_deactivate(GTK_MENU_SHELL(self->priv->menu));
  }
}

/**
 * update_indicator_visibility:
 * @self: the indicator object
 *
 * Changes the visibility of the indicator image based on the value
 * of hide_indicator.
 **/
static void
update_indicator_visibility(IndicatorNotifications *self)
{
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(self));
  
  if(self->priv->image != NULL) {
    if(self->priv->hide_indicator)
      gtk_widget_hide(GTK_WIDGET(self->priv->image));
    else
      gtk_widget_show(GTK_WIDGET(self->priv->image));
  }
}

/**
 * clear_item_activated_cb:
 * @menuitem: the clear menuitem
 * @user_data: the indicator object
 *
 * Called when the clear menuitem is activated.
 **/
static void
clear_item_activated_cb(GtkMenuItem *menuitem, gpointer user_data)
{
  g_return_if_fail(GTK_IS_MENU_ITEM(menuitem));
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(user_data));
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);

  clear_menuitems(self);
}

/**
 * setting_changed_cb:
 * @settings: the GSettings object
 * @key: the GSettings key
 * @user_data: the indicator object
 *
 * Called when a GSettings key is changed.
 **/
static void
setting_changed_cb(GSettings *settings, gchar *key, gpointer user_data)
{
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(user_data));
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);

  if(g_strcmp0(key, NOTIFICATIONS_KEY_HIDE_INDICATOR) == 0) {
    self->priv->hide_indicator = g_settings_get_boolean(settings, NOTIFICATIONS_KEY_HIDE_INDICATOR);
    update_indicator_visibility(self);
  }
  else if(g_strcmp0(key, NOTIFICATIONS_KEY_BLACKLIST) == 0) {
    update_blacklist(self);
  }
  /* TODO: Trim or extend the notifications list based on "max-items" key
   * (Currently requires a restart) */
}

/**
 * menu_visible_notify_cb:
 * @menu: the menu
 * @pspec: unused
 * @user_data: the indicator object
 *
 * Called when the indicator's menu is shown or hidden.
 **/
static void
menu_visible_notify_cb(GtkWidget *menu, G_GNUC_UNUSED GParamSpec *pspec, gpointer user_data)
{
  g_return_if_fail(GTK_IS_MENU(menu));
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(user_data));
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);

  gboolean visible;
  g_object_get(G_OBJECT(menu), "visible", &visible, NULL);
  if(!visible) {
    set_unread(self, FALSE);
  }
}

/**
 * message_received_cb:
 * @spy: the dbus notification monitor
 * @note: the notification received
 * @user_data: the indicator object
 *
 * Called when a notification arrives on dbus.
 **/
static void
message_received_cb(DBusSpy *spy, Notification *note, gpointer user_data)
{
  g_return_if_fail(IS_DBUS_SPY(spy));
  g_return_if_fail(IS_NOTIFICATION(note));
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(user_data));
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);

  /* Discard useless notifications */
  if(notification_is_private(note) || notification_is_empty(note)) {
    g_object_unref(note);
    return;
  }

  /* Discard notifications on the blacklist */
  if(self->priv->blacklist != NULL && g_hash_table_contains(self->priv->blacklist,
        notification_get_app_name(note))) {
    g_object_unref(note);
    return;
  }

  GtkWidget *item = notification_menuitem_new();
  notification_menuitem_set_from_notification(NOTIFICATION_MENUITEM(item), note);
  g_signal_connect(item, NOTIFICATION_MENUITEM_SIGNAL_CLICKED, G_CALLBACK(notification_clicked_cb), self);
  gtk_widget_show(item);
  g_object_unref(note);

  insert_menuitem(self, item);

  set_unread(self, TRUE);
}

/**
 * notification_clicked_cb:
 * @widget: the menuitem
 * @user_data: the indicator object
 *
 * Remove the menuitem when clicked.
 **/
static void
notification_clicked_cb(NotificationMenuItem *menuitem, guint button, gpointer user_data)
{
  g_return_if_fail(IS_NOTIFICATION_MENUITEM(menuitem));
  g_return_if_fail(IS_INDICATOR_NOTIFICATIONS(user_data));
  IndicatorNotifications *self = INDICATOR_NOTIFICATIONS(user_data);

  remove_menuitem(self, GTK_WIDGET(menuitem));
}
