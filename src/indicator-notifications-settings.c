#include <gtk/gtk.h>

#include "settings.h"

#define SCHEMA_KEY "schema-key"

#define COLUMN_APPNAME 0

typedef struct
{
  GtkApplication parent_instance;

  GSettings *settings;

  GtkWidget *blacklist_treeview;
  GtkWidget *blacklist_entry;

  /* GtkTreeModel foreach variables */
  gboolean result;
  gchar *text;
  GPtrArray *array;
} IndicatorNotificationsSettings;

typedef GtkApplicationClass IndicatorNotificationsSettingsClass;

G_DEFINE_TYPE(IndicatorNotificationsSettings, indicator_notifications_settings, GTK_TYPE_APPLICATION)

/* Class Functions */
static void indicator_notifications_settings_class_init(IndicatorNotificationsSettingsClass *klass);
static void indicator_notifications_settings_init(IndicatorNotificationsSettings *self);
static void indicator_notifications_settings_dispose(GObject *object);

/* GtkApplication Signals */
static void indicator_notifications_settings_activate(GApplication *app);

/* Utility Functions */
static void load_blacklist(IndicatorNotificationsSettings *self);
static void save_blacklist(IndicatorNotificationsSettings *self);
static gboolean foreach_check_duplicates(GtkTreeModel *model, GtkTreePath *path,
                                         GtkTreeIter *iter, gpointer user_data);
static gboolean foreach_build_array(GtkTreeModel *model, GtkTreePath *path,
                                    GtkTreeIter *iter, gpointer user_data);

/* Callbacks */
static void blacklist_add_clicked_cb(GtkButton *button, gpointer user_data);
static void blacklist_remove_clicked_cb(GtkButton *button, gpointer user_data);
static void button_toggled_cb(GtkToggleButton *button, gpointer user_data);
static void max_items_changed_cb(GtkSpinButton *button, gpointer user_data);

static void
load_blacklist(IndicatorNotificationsSettings *self)
{
  GtkListStore *list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->blacklist_treeview)));
  GtkTreeIter iter;
  gchar **items;

  gtk_list_store_clear(list);

  items = g_settings_get_strv(self->settings, NOTIFICATIONS_KEY_BLACKLIST);

  for (int i = 0; items[i] != NULL; i++) {
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, COLUMN_APPNAME, items[i], -1);
  }

  g_strfreev(items);
}

static void
save_blacklist(IndicatorNotificationsSettings *self)
{
  GtkListStore *list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->blacklist_treeview)));
  gchar **items;

  /* build an array of the blacklist items */
  self->array = g_ptr_array_new();
  gtk_tree_model_foreach(GTK_TREE_MODEL(list), foreach_build_array, self);
  g_ptr_array_add(self->array, NULL);
  items = (gchar **) g_ptr_array_free(self->array, FALSE);
  self->array = NULL;

  g_settings_set_strv(self->settings, NOTIFICATIONS_KEY_BLACKLIST, (const gchar **) items);

  g_strfreev(items);
}

static gboolean
foreach_check_duplicates(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) user_data;
  gchar *appname;
  gboolean result = FALSE;

  gtk_tree_model_get(model, iter, COLUMN_APPNAME, &appname, -1);

  if (g_strcmp0(appname, self->text) == 0) {
    result = TRUE;
    self->result = TRUE;
  }

  g_free(appname);

  return result;
}

static gboolean
foreach_build_array(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) user_data;
  gchar *appname;

  gtk_tree_model_get(model, iter, COLUMN_APPNAME, &appname, -1);

  g_ptr_array_add(self->array, appname);

  return FALSE;
}

static void
blacklist_add_clicked_cb(GtkButton *button, gpointer user_data)
{
  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) user_data;
  GtkListStore *list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->blacklist_treeview)));
  GtkTreeIter iter;

  /* strip off the leading and trailing whitespace in case of user error */
  self->text = g_strdup(gtk_entry_get_text(GTK_ENTRY(self->blacklist_entry)));
  g_strstrip(self->text);

  if (strlen(self->text) > 0) {
    /* check for duplicates first */
    self->result = FALSE;
    gtk_tree_model_foreach(GTK_TREE_MODEL(list), foreach_check_duplicates, self);

    if (self->result == FALSE) {
      gtk_list_store_append(list, &iter);
      gtk_list_store_set(list, &iter, COLUMN_APPNAME, self->text, -1);
      save_blacklist(self);
    }
  }

  /* clear the entry */
  gtk_entry_set_text(GTK_ENTRY(self->blacklist_entry), "");

  /* cleanup text */
  g_free(self->text);
  self->text = "";
}

static void
blacklist_remove_clicked_cb(GtkButton *button, gpointer user_data)
{
  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) user_data;
  GtkListStore *list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->blacklist_treeview)));
  GtkTreeIter iter;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->blacklist_treeview));

  if (gtk_tree_selection_get_selected(selection, NULL, &iter) == TRUE) {
    gtk_list_store_remove(list, &iter);

    save_blacklist(self);
  }
}

static void
button_toggled_cb(GtkToggleButton *button, gpointer user_data)
{
  GSettings *settings = G_SETTINGS(user_data);
  char *schema_key = (char *) g_object_get_data(G_OBJECT(button), SCHEMA_KEY);

  g_settings_set_boolean(settings, schema_key, gtk_toggle_button_get_active(button));
}

static void
max_items_changed_cb(GtkSpinButton *button, gpointer user_data)
{
  GSettings *settings = G_SETTINGS(user_data);

  int value = gtk_spin_button_get_value_as_int(button);
  g_settings_set_int(settings, NOTIFICATIONS_KEY_MAX_ITEMS, value);
}

static void
indicator_notifications_settings_activate(GApplication *app)
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button_1;
  GtkWidget *button_2;
  GtkWidget *spin;
  GtkWidget *spin_label;
  GtkWidget *blacklist_label;
  GtkListStore *blacklist_list;
  GtkWidget *blacklist_scroll;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *hbox;
  GtkWidget *button_3;
  GtkWidget *button_4;

  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) app;

  /* GSettings */
  self->settings = g_settings_new(NOTIFICATIONS_SCHEMA);

  /* Main Window */
  window = gtk_application_window_new(GTK_APPLICATION(app));
  gtk_window_set_title(GTK_WINDOW(window), "Indicator Notifications Settings");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  gtk_widget_show(window);

  /* Window Frame */
  frame = gtk_frame_new("Indicator Notifications Settings");
  gtk_container_add(GTK_CONTAINER(window), frame);
  gtk_widget_show(frame);

  /* Main Vertical Box */
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);

  /* clear-on-middle-click */
  button_1 = gtk_check_button_new_with_label("Clear notifications on middle click");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_1),
      g_settings_get_boolean(self->settings, NOTIFICATIONS_KEY_CLEAR_MC));
  g_object_set_data(G_OBJECT(button_1), SCHEMA_KEY, NOTIFICATIONS_KEY_CLEAR_MC);
  g_signal_connect(button_1, "toggled", G_CALLBACK(button_toggled_cb), self->settings);
  gtk_box_pack_start(GTK_BOX(vbox), button_1, FALSE, FALSE, 4);
  gtk_widget_show(button_1);

  /* hide-indicator */
  button_2 = gtk_check_button_new_with_label("Hide indicator");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_2),
      g_settings_get_boolean(self->settings, NOTIFICATIONS_KEY_HIDE_INDICATOR));
  g_object_set_data(G_OBJECT(button_2), SCHEMA_KEY, NOTIFICATIONS_KEY_HIDE_INDICATOR);
  g_signal_connect(button_2, "toggled", G_CALLBACK(button_toggled_cb), self->settings);
  gtk_box_pack_start(GTK_BOX(vbox), button_2, FALSE, FALSE, 4);
  gtk_widget_show(button_2);

  /* max-items */
  /* FIXME: indicator does not change max items until restart... */
  spin_label = gtk_label_new("Maximum number of visible notifications");
  gtk_box_pack_start(GTK_BOX(vbox), spin_label, FALSE, FALSE, 4);
  gtk_widget_show(spin_label);

  spin = gtk_spin_button_new_with_range(1, 10, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), g_settings_get_int(self->settings, NOTIFICATIONS_KEY_MAX_ITEMS));
  g_signal_connect(spin, "value-changed", G_CALLBACK(max_items_changed_cb), self->settings);
  gtk_box_pack_start(GTK_BOX(vbox), spin, FALSE, FALSE, 4);
  gtk_widget_show(spin);

  /* blacklist */
  blacklist_label = gtk_label_new("Discard notifications by application name");
  gtk_box_pack_start(GTK_BOX(vbox), blacklist_label, FALSE, FALSE, 4);
  gtk_widget_show(blacklist_label);

  blacklist_scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(vbox), blacklist_scroll, TRUE, TRUE, 4);
  gtk_widget_show(blacklist_scroll);

  blacklist_list = gtk_list_store_new(1, G_TYPE_STRING);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("appname", renderer, "text", COLUMN_APPNAME, NULL);

  self->blacklist_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(blacklist_list));
  g_object_unref(blacklist_list);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->blacklist_treeview), FALSE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(self->blacklist_treeview), column);
  load_blacklist(self);
  gtk_container_add(GTK_CONTAINER(blacklist_scroll), self->blacklist_treeview);
  gtk_widget_show(self->blacklist_treeview);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);

  button_3 = gtk_button_new_with_label("Remove");
  g_signal_connect(button_3, "clicked", G_CALLBACK(blacklist_remove_clicked_cb), self);
  gtk_box_pack_start(GTK_BOX(hbox), button_3, FALSE, FALSE, 2);
  gtk_widget_show(button_3);

  button_4 = gtk_button_new_with_label("Add");
  g_signal_connect(button_4, "clicked", G_CALLBACK(blacklist_add_clicked_cb), self);
  gtk_box_pack_start(GTK_BOX(hbox), button_4, FALSE, FALSE, 2);
  gtk_widget_show(button_4);

  self->blacklist_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), self->blacklist_entry, TRUE, TRUE, 0);
  gtk_widget_show(self->blacklist_entry);
}

static void
indicator_notifications_settings_init (IndicatorNotificationsSettings *self)
{
}

static void
indicator_notifications_settings_class_init (IndicatorNotificationsSettingsClass *class)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  application_class->activate = indicator_notifications_settings_activate;

  object_class->dispose = indicator_notifications_settings_dispose;
}

static void
indicator_notifications_settings_dispose(GObject *object)
{
  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) object;

  if(self->settings != NULL) {
    g_object_unref(G_OBJECT(self->settings));
    self->settings = NULL;
  }

  G_OBJECT_CLASS(indicator_notifications_settings_parent_class)->dispose(object);
}

IndicatorNotificationsSettings *
indicator_notifications_settings_new (void)
{
  IndicatorNotificationsSettings *self;

  g_set_application_name("Indicator Notifications Settings");

  self = g_object_new(indicator_notifications_settings_get_type(),
    "application-id", NOTIFICATIONS_SCHEMA ".settings",
    "flags", G_APPLICATION_FLAGS_NONE,
    NULL);

  return self;
}

int
main(int argc, char **argv)
{
  IndicatorNotificationsSettings *self;
  int status;

  self = indicator_notifications_settings_new();

  status = g_application_run(G_APPLICATION(self), argc, argv);

  g_object_unref(self);

  return status;
}
