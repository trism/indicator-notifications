/*
An example indicator.

Copyright 2010 Canonical Ltd.

Authors:
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

#include "example-interface.h"
#include "dbus-shared.h"
#include "settings-shared.h"

static IndicatorService *service = NULL;
static GMainLoop *mainloop = NULL;
static DbusmenuServer *server = NULL;
static DbusmenuMenuitem *root = NULL;
static ExampleInterface *dbus = NULL;

/* Global Items */
static DbusmenuMenuitem *item_1 = NULL;
static DbusmenuMenuitem *item_2 = NULL;

static void
build_menus(DbusmenuMenuitem *root)
{
  g_debug("Building Menus.");
  if (item_1 == NULL) {
    item_1 = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(item_1, DBUSMENU_MENUITEM_PROP_LABEL, _("Test Item 1"));
    dbusmenu_menuitem_child_append(root, item_1);
  }
  if (item_2 == NULL) {
    item_2 = dbusmenu_menuitem_new();
    dbusmenu_menuitem_property_set(item_2, DBUSMENU_MENUITEM_PROP_LABEL, _("Test Item 2"));
    dbusmenu_menuitem_child_append(root, item_2);
  }

  return;
}

/* Responds to the service object saying it's time to shutdown.
   It stops the mainloop. */
static void 
service_shutdown(IndicatorService *service, gpointer user_data)
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

  /* Acknowledging the service init and setting up the interface */
  service = indicator_service_new_version(SERVICE_NAME, SERVICE_VERSION);
  g_signal_connect(service, INDICATOR_SERVICE_SIGNAL_SHUTDOWN, G_CALLBACK(service_shutdown), NULL);

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

  /* Setup dbus interface */
  dbus = g_object_new(EXAMPLE_INTERFACE_TYPE, NULL);

  mainloop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(mainloop);

  g_object_unref(G_OBJECT(dbus));
  g_object_unref(G_OBJECT(service));
  g_object_unref(G_OBJECT(server));
  g_object_unref(G_OBJECT(root));

  return 0;
}
