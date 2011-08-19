/*
An indicator to display recent notifications.

Adapted from: indicator-datetime/src/datetime-service.c by
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

#include <config.h>
#include <libindicator/indicator-service.h>
#include <locale.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#if WITH_GTK == 3
#include <libdbusmenu-gtk3/menuitem.h>
#else
#include <libdbusmenu-gtk/menuitem.h>
#endif

#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-glib/client.h>
#include <libdbusmenu-glib/menuitem.h>

#include "dbus-shared.h"
#include "dbus-spy.h"
#include "notifications-interface.h"
#include "settings-shared.h"

static IndicatorService *service = NULL;
static GMainLoop *mainloop = NULL;
static DbusmenuServer *server = NULL;
static DbusmenuMenuitem *root = NULL;
static DBusSpy *spy = NULL;
static NotificationsInterface *dbus = NULL;

/* Global Items */
static DbusmenuMenuitem *clear_item = NULL;
static DbusmenuMenuitem *empty_item = NULL;
static GQueue *notification_items = NULL;
static guint notification_limit = 5;

/* Logging */
#define LOG_FILE_NAME "indicator-notifications-service.log"
static FILE *log_file = NULL;

static gboolean add_notification_item(gpointer);
static gboolean clear_notification_items(gpointer);
static void build_menus(DbusmenuMenuitem *);
static void clear_notifications_cb(DbusmenuMenuitem *, guint, gpointer);
static void log_cb(const gchar *, GLogLevelFlags, const gchar *, gpointer);
static void log_init();
static void message_received_cb(DBusSpy *, Notification *, gpointer);
static void service_shutdown_cb(IndicatorService *, gpointer);

static gboolean
add_notification_item(gpointer user_data)
{
  Notification *note = NOTIFICATION(user_data);
  DbusmenuMenuitem *item;

  guint length = g_queue_get_length(notification_items);

  /* Remove the empty item from the menu */
  if(length == 0) {
    dbusmenu_menuitem_child_delete(root, empty_item);
  }

  gchar *timestamp_string = notification_timestamp_for_locale(note);

  item = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TYPE, NOTIFICATION_MENUITEM_TYPE);
  dbusmenu_menuitem_property_set(item, NOTIFICATION_MENUITEM_PROP_APP_NAME, notification_get_app_name(note));
  dbusmenu_menuitem_property_set(item, NOTIFICATION_MENUITEM_PROP_SUMMARY, notification_get_summary(note));
  dbusmenu_menuitem_property_set(item, NOTIFICATION_MENUITEM_PROP_BODY, notification_get_body(note));
  dbusmenu_menuitem_property_set(item, NOTIFICATION_MENUITEM_PROP_TIMESTAMP_STRING, timestamp_string);
  dbusmenu_menuitem_child_prepend(root, item);
  g_queue_push_head(notification_items, item);
  length++;

  g_debug("Adding message from %s (Queue length: %d)", notification_get_app_name(note),
      length);

  if(length > notification_limit) {
    item = DBUSMENU_MENUITEM(g_queue_pop_tail(notification_items));
    dbusmenu_menuitem_child_delete(root, item);
    g_object_unref(item);
    item = NULL;
  }

  /* Notify the indicator that a new message has been added */
  notifications_interface_message_added(dbus);

  g_free(timestamp_string);
  g_object_unref(note);

  return FALSE;
}

static gboolean
clear_notification_items(gpointer user_data)
{
  DbusmenuMenuitem *item;

  while(!g_queue_is_empty(notification_items)) {
    item = DBUSMENU_MENUITEM(g_queue_pop_tail(notification_items));
    dbusmenu_menuitem_child_delete(root, item);
    g_object_unref(item);
  }

  item = NULL;

  /* Add the empty item back, if it isn't already there */
  if(dbusmenu_menuitem_child_find(root, dbusmenu_menuitem_get_id(empty_item)) == NULL) {
    dbusmenu_menuitem_child_prepend(root, empty_item);
  }

  return FALSE;
}

static void
build_menus(DbusmenuMenuitem *root)
{
  g_debug("Building Menus.");

  if(empty_item == NULL) {
    empty_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(empty_item, DBUSMENU_MENUITEM_PROP_LABEL, _("There are 0 notifications."));
    dbusmenu_menuitem_child_append(root, empty_item);
  }

  if(clear_item == NULL) {
    DbusmenuMenuitem *item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TYPE, DBUSMENU_CLIENT_TYPES_SEPARATOR);
    dbusmenu_menuitem_child_append(root, item);

    clear_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(clear_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Clear"));
    dbusmenu_menuitem_child_append(root, clear_item);

    g_signal_connect(G_OBJECT(clear_item), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, 
        G_CALLBACK(clear_notifications_cb), NULL);
  }

  return;
}

static void
clear_notifications_cb(DbusmenuMenuitem *item, guint timestamp, gpointer user_data)
{
  g_idle_add(clear_notification_items, NULL);
}

/* from lightdm */
static void
log_cb(const gchar *domain, GLogLevelFlags level, const gchar *message, gpointer user_data)
{
  if(log_file) {
    const gchar *prefix;

    switch(level & G_LOG_LEVEL_MASK) {
      case G_LOG_LEVEL_ERROR:
        prefix = "ERROR:";
        break;
      case G_LOG_LEVEL_CRITICAL:
        prefix = "CRITICAL:";
        break;
      case G_LOG_LEVEL_WARNING:
        prefix = "WARNING:";
        break;
      case G_LOG_LEVEL_MESSAGE:
        prefix = "MESSAGE:";
        break;
      case G_LOG_LEVEL_INFO:
        prefix = "INFO:";
        break;
      case G_LOG_LEVEL_DEBUG:
        prefix = "DEBUG:";
        break;
      default:
        prefix = "LOG:";
        break;
    }

    fprintf(log_file, "%s %s\n", prefix, message);
    fflush(log_file);
  }

  g_log_default_handler(domain, level, message, user_data);
}

/* from lightdm */
static void
log_init()
{
  gchar *path;

  g_mkdir_with_parents(g_get_user_cache_dir(), 0755);
  path = g_build_filename(g_get_user_cache_dir(), LOG_FILE_NAME, NULL);

  log_file = fopen(path, "w");
  g_log_set_default_handler(log_cb, NULL);

  g_debug("Logging to %s", path);
  g_free(path);
}

static void
message_received_cb(DBusSpy *spy, Notification *note, gpointer user_data)
{
  /* Discard volume notifications */
  if(notification_is_volume(note)) return;

  g_object_ref(note);
  g_idle_add(add_notification_item, note);
}

/* Responds to the service object saying it's time to shutdown.
   It stops the mainloop. */
static void 
service_shutdown_cb(IndicatorService *service, gpointer user_data)
{
  g_warning("Shutting down service!");
  g_main_loop_quit(mainloop);
  return;
}

/* Function to build everything up.  Entry point from asm. */
int
main(int argc, char **argv)
{
  g_type_init();

  /* Logging */
  log_init();

  /* Acknowledging the service init and setting up the interface */
  service = indicator_service_new_version(SERVICE_NAME, SERVICE_VERSION);
  g_signal_connect(service, INDICATOR_SERVICE_SIGNAL_SHUTDOWN, G_CALLBACK(service_shutdown_cb), NULL);

  /* Setting up i18n and gettext.  Apparently, we need
  all of these. */
  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
  textdomain(GETTEXT_PACKAGE);

  /* Building the base menu */
  server = dbusmenu_server_new(MENU_OBJ);
  root = dbusmenu_menuitem_new();
  dbusmenu_server_set_root(server, root);

  build_menus(root);

  /* Create the notification queue */
  notification_items = g_queue_new();

  /* Set up the notification spy */
  spy = dbus_spy_new();
  g_signal_connect(spy, DBUS_SPY_SIGNAL_MESSAGE_RECEIVED, G_CALLBACK(message_received_cb), NULL);

  /* Setup the dbus interface */
  dbus = notifications_interface_new();

  mainloop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(mainloop);

  g_object_unref(G_OBJECT(dbus));
  g_object_unref(G_OBJECT(spy));
  g_object_unref(G_OBJECT(service));
  g_object_unref(G_OBJECT(server));
  g_object_unref(G_OBJECT(root));

  fclose(log_file);

  return 0;
}
