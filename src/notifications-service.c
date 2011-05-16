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

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <libdbusmenu-gtk/menuitem.h>
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
static DbusmenuMenuitem *filter_item = NULL;
static DbusmenuMenuitem *all_applications_item = NULL;
static GQueue *notification_items = NULL;
static guint notification_limit = 5;

/* Logging */
#define LOG_FILE_NAME "indicator-notifications-service.log"
static GOutputStream *log_file = NULL;

static gboolean add_notification_item(gpointer);
static gboolean clear_notification_items(gpointer);
static void build_menus(DbusmenuMenuitem *);
static void clear_notifications_cb(DbusmenuMenuitem *, guint, gpointer);
static void log_to_file_cb(GObject *, GAsyncResult *, gpointer);
static void log_to_file(const gchar *, GLogLevelFlags, const gchar *, gpointer);
static void message_received_cb(DBusSpy *, Notification *, gpointer);
static void service_shutdown_cb(IndicatorService *, gpointer);

static gboolean
add_notification_item(gpointer user_data)
{
  Notification *note = NOTIFICATION(user_data);
  DbusmenuMenuitem *item;

  item = dbusmenu_menuitem_new();
  dbusmenu_menuitem_property_set(item, DBUSMENU_MENUITEM_PROP_TYPE, NOTIFICATION_MENUITEM_TYPE);
  dbusmenu_menuitem_property_set(item, NOTIFICATION_MENUITEM_PROP_APP_NAME, notification_get_app_name(note));
  dbusmenu_menuitem_property_set(item, NOTIFICATION_MENUITEM_PROP_SUMMARY, notification_get_summary(note));
  dbusmenu_menuitem_property_set(item, NOTIFICATION_MENUITEM_PROP_BODY, notification_get_body(note));
  dbusmenu_menuitem_child_add_position(root, item, 1);
  g_queue_push_head(notification_items, item);

  guint length = g_queue_get_length(notification_items);

  g_debug("Adding message from %s (Queue length: %d)", notification_get_app_name(note),
      length);

  if(length > notification_limit) {
    item = DBUSMENU_MENUITEM(g_queue_pop_tail(notification_items));
    dbusmenu_menuitem_child_delete(root, item);
    g_object_unref(item);
    item = NULL;
  }

  notifications_interface_message_added(dbus);

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

  return FALSE;
}

static void
build_menus(DbusmenuMenuitem *root)
{
  g_debug("Building Menus.");
  if(clear_item == NULL) {
    clear_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(clear_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Clear"));
    dbusmenu_menuitem_child_prepend(root, clear_item);

    g_signal_connect(G_OBJECT(clear_item), DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED, 
        G_CALLBACK(clear_notifications_cb), NULL);
  }
  if(filter_item == NULL) {
    filter_item = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(filter_item, DBUSMENU_MENUITEM_PROP_LABEL, _("Filter"));
    dbusmenu_menuitem_child_prepend(root, filter_item);

    if(all_applications_item == NULL) {
      all_applications_item = dbusmenu_menuitem_new();
      dbusmenu_menuitem_property_set(all_applications_item, DBUSMENU_MENUITEM_PROP_LABEL, _("All Applications"));
      dbusmenu_menuitem_child_append(filter_item, all_applications_item);
    }
  }

  return;
}

static void
clear_notifications_cb(DbusmenuMenuitem *item, guint timestamp, gpointer user_data)
{
  g_idle_add(clear_notification_items, NULL);
}

/* from indicator-applet */
static void
log_to_file_cb(GObject *source_obj, GAsyncResult *result, gpointer user_data)
{
  g_free(user_data);
}

/* from indicator-applet */
static void
log_to_file(const gchar *domain, GLogLevelFlags level, const gchar *message, gpointer user_data)
{
  /* Create the log file */
  if(log_file == NULL) {
    GError *error = NULL;
    gchar *filename = g_build_filename(g_get_user_cache_dir(), LOG_FILE_NAME, NULL);
    GFile *file = g_file_new_for_path(filename);
    g_free(filename);

    /* Create the ~/.cache directory if it doesn't exist */
    if(!g_file_test(g_get_user_cache_dir(), G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
      GFile *cache_dir = g_file_new_for_path(g_get_user_cache_dir());
      g_file_make_directory_with_parents(cache_dir, NULL, &error);

      if(error != NULL) {
        g_error("Unable to make directory '%s' for log file: %s", g_get_user_cache_dir(), error->message);
        return;
      }
    }

    g_file_delete(file, NULL, NULL);

    GFileIOStream *io = g_file_create_readwrite(file,
        G_FILE_CREATE_REPLACE_DESTINATION,
        NULL,
        &error);

    if(error != NULL) {
      g_error("Unable to replace file: %s", error->message);
      return;
    }

    log_file = g_io_stream_get_output_stream(G_IO_STREAM(io));
  }

  gchar *output_string = g_strdup_printf("%s\n", message);
  g_output_stream_write_async(log_file,
      output_string,
      strlen(output_string),
      G_PRIORITY_LOW,
      NULL,
      log_to_file_cb,
      output_string);
}

static void
message_received_cb(DBusSpy *spy, Notification *note, gpointer user_data)
{
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
  g_log_set_default_handler(log_to_file, NULL);

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

  return 0;
}
