/*
 * indicator-notifications-settings.c - UI for indicator settings
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "settings.h"

#define SCHEMA_KEY "schema-key"

#define COLUMN_APPNAME 0

typedef struct
{
  GtkApplication parent_instance;

  GSettings *settings;

  GtkWidget *filter_list_treeview;
  GtkWidget *filter_list_entry;

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
static void load_filter_list(IndicatorNotificationsSettings *self);
static void load_filter_list_hints(IndicatorNotificationsSettings *self);
static void save_filter_list(IndicatorNotificationsSettings *self);
static gboolean foreach_check_duplicates(GtkTreeModel *model, GtkTreePath *path,
                                         GtkTreeIter *iter, gpointer user_data);
static gboolean foreach_build_array(GtkTreeModel *model, GtkTreePath *path,
                                    GtkTreeIter *iter, gpointer user_data);

/* Callbacks */
static void filter_list_add_clicked_cb(GtkButton *button, gpointer user_data);
static void filter_list_remove_clicked_cb(GtkButton *button, gpointer user_data);
static void button_toggled_cb(GtkToggleButton *button, gpointer user_data);
static void max_items_changed_cb(GtkSpinButton *button, gpointer user_data);
static gboolean filter_list_entry_focus_in_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data);

static void
load_filter_list(IndicatorNotificationsSettings *self)
{
  GtkListStore *list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->filter_list_treeview)));
  GtkTreeIter iter;
  gchar **items;

  gtk_list_store_clear(list);

  items = g_settings_get_strv(self->settings, NOTIFICATIONS_KEY_FILTER_LIST);

  for (int i = 0; items[i] != NULL; i++) {
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, COLUMN_APPNAME, items[i], -1);
  }

  g_strfreev(items);
}

static void
load_filter_list_hints(IndicatorNotificationsSettings *self)
{
  GtkEntryCompletion *completion = gtk_entry_get_completion(GTK_ENTRY(self->filter_list_entry));
  GtkListStore *list = GTK_LIST_STORE(gtk_entry_completion_get_model(completion));
  GtkTreeIter iter;
  gchar **items;

  gtk_list_store_clear(list);

  items = g_settings_get_strv(self->settings, NOTIFICATIONS_KEY_FILTER_LIST_HINTS);

  for (int i = 0; items[i] != NULL; i++) {
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, 0, items[i], -1);
  }

  g_strfreev(items);
}

static void
save_filter_list(IndicatorNotificationsSettings *self)
{
  GtkListStore *list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->filter_list_treeview)));
  gchar **items;

  /* build an array of the filter list items */
  self->array = g_ptr_array_new();
  gtk_tree_model_foreach(GTK_TREE_MODEL(list), foreach_build_array, self);
  g_ptr_array_add(self->array, NULL);
  items = (gchar **) g_ptr_array_free(self->array, FALSE);
  self->array = NULL;

  g_settings_set_strv(self->settings, NOTIFICATIONS_KEY_FILTER_LIST, (const gchar **) items);

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
filter_list_add_clicked_cb(GtkButton *button, gpointer user_data)
{
  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) user_data;
  GtkListStore *list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->filter_list_treeview)));
  GtkTreeIter iter;

  /* strip off the leading and trailing whitespace in case of user error */
  self->text = g_strdup(gtk_entry_get_text(GTK_ENTRY(self->filter_list_entry)));
  g_strstrip(self->text);

  if (strlen(self->text) > 0) {
    /* check for duplicates first */
    self->result = FALSE;
    gtk_tree_model_foreach(GTK_TREE_MODEL(list), foreach_check_duplicates, self);

    if (self->result == FALSE) {
      gtk_list_store_append(list, &iter);
      gtk_list_store_set(list, &iter, COLUMN_APPNAME, self->text, -1);
      save_filter_list(self);
    }
  }

  /* clear the entry */
  gtk_entry_set_text(GTK_ENTRY(self->filter_list_entry), "");

  /* cleanup text */
  g_free(self->text);
  self->text = "";
}

static void
filter_list_remove_clicked_cb(GtkButton *button, gpointer user_data)
{
  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) user_data;
  GtkListStore *list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(self->filter_list_treeview)));
  GtkTreeIter iter;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->filter_list_treeview));

  if (gtk_tree_selection_get_selected(selection, NULL, &iter) == TRUE) {
    gtk_list_store_remove(list, &iter);

    save_filter_list(self);
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

static gboolean
filter_list_entry_focus_in_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) user_data;
  load_filter_list_hints(self);
  g_signal_emit_by_name(widget, "changed", NULL);
  return FALSE;
}

static void
indicator_notifications_settings_activate(GApplication *app)
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *button_cmc;
  GtkWidget *button_hide_ind;
  GtkWidget *button_dnd;
  GtkWidget *spin;
  GtkWidget *spin_label;
  GtkWidget *filter_list_label;
  GtkListStore *filter_list_list;
  GtkWidget *filter_list_scroll;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *hbox;
  GtkWidget *button_filter_list_rem;
  GtkWidget *button_filter_list_add;
  GtkEntryCompletion *entry_completion;
  GtkListStore *entry_list;

  IndicatorNotificationsSettings *self = (IndicatorNotificationsSettings *) app;

  /* Check for a pre-existing window */
  GtkWindow *old_window = gtk_application_get_active_window(GTK_APPLICATION(app));
  if (old_window != NULL) {
    gtk_window_present_with_time(old_window, GDK_CURRENT_TIME);
    return;
  }

  /* GSettings */
  self->settings = g_settings_new(NOTIFICATIONS_SCHEMA);

  /* Main Window */
  window = gtk_application_window_new(GTK_APPLICATION(app));
  gtk_window_set_title(GTK_WINDOW(window), _("Indicator Notifications Settings"));
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  gtk_widget_show(window);

  /* Window Frame */
  frame = gtk_frame_new(_("Indicator Notifications Settings"));
  gtk_container_add(GTK_CONTAINER(window), frame);
  gtk_widget_show(frame);

  /* Main Vertical Box */
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);

  /* clear-on-middle-click */
  button_cmc = gtk_check_button_new_with_label(_("Clear notifications on middle click"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_cmc),
      g_settings_get_boolean(self->settings, NOTIFICATIONS_KEY_CLEAR_MC));
  g_object_set_data(G_OBJECT(button_cmc), SCHEMA_KEY, NOTIFICATIONS_KEY_CLEAR_MC);
  g_signal_connect(button_cmc, "toggled", G_CALLBACK(button_toggled_cb), self->settings);
  gtk_box_pack_start(GTK_BOX(vbox), button_cmc, FALSE, FALSE, 4);
  gtk_widget_show(button_cmc);

  /* hide-indicator */
  button_hide_ind = gtk_check_button_new_with_label(_("Hide indicator"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_hide_ind),
      g_settings_get_boolean(self->settings, NOTIFICATIONS_KEY_HIDE_INDICATOR));
  g_object_set_data(G_OBJECT(button_hide_ind), SCHEMA_KEY, NOTIFICATIONS_KEY_HIDE_INDICATOR);
  g_signal_connect(button_hide_ind, "toggled", G_CALLBACK(button_toggled_cb), self->settings);
  gtk_box_pack_start(GTK_BOX(vbox), button_hide_ind, FALSE, FALSE, 4);
  gtk_widget_show(button_hide_ind);

  /* do-not-disturb */
  button_dnd = gtk_check_button_new_with_label(_("Enable do not disturb"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_dnd),
      g_settings_get_boolean(self->settings, NOTIFICATIONS_KEY_DND));
  g_object_set_data(G_OBJECT(button_dnd), SCHEMA_KEY, NOTIFICATIONS_KEY_DND);
  g_signal_connect(button_dnd, "toggled", G_CALLBACK(button_toggled_cb), self->settings);
  gtk_box_pack_start(GTK_BOX(vbox), button_dnd, FALSE, FALSE, 4);
  gtk_widget_show(button_dnd);

  /* max-items */
  /* FIXME: indicator does not change max items until restart... */
  spin_label = gtk_label_new(_("Maximum number of visible notifications"));
  gtk_box_pack_start(GTK_BOX(vbox), spin_label, FALSE, FALSE, 4);
  gtk_widget_show(spin_label);

  spin = gtk_spin_button_new_with_range(1, 10, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), g_settings_get_int(self->settings, NOTIFICATIONS_KEY_MAX_ITEMS));
  g_signal_connect(spin, "value-changed", G_CALLBACK(max_items_changed_cb), self->settings);
  gtk_box_pack_start(GTK_BOX(vbox), spin, FALSE, FALSE, 4);
  gtk_widget_show(spin);

  /* filter-list */
  filter_list_label = gtk_label_new(_("Discard notifications by application name"));
  gtk_box_pack_start(GTK_BOX(vbox), filter_list_label, FALSE, FALSE, 4);
  gtk_widget_show(filter_list_label);

  filter_list_scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(vbox), filter_list_scroll, TRUE, TRUE, 4);
  gtk_widget_show(filter_list_scroll);

  filter_list_list = gtk_list_store_new(1, G_TYPE_STRING);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("appname", renderer, "text", COLUMN_APPNAME, NULL);

  self->filter_list_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(filter_list_list));
  g_object_unref(filter_list_list);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self->filter_list_treeview), FALSE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(self->filter_list_treeview), column);
  load_filter_list(self);
  gtk_container_add(GTK_CONTAINER(filter_list_scroll), self->filter_list_treeview);
  gtk_widget_show(self->filter_list_treeview);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);

  button_filter_list_rem = gtk_button_new_with_label(_("Remove"));
  g_signal_connect(button_filter_list_rem, "clicked", G_CALLBACK(filter_list_remove_clicked_cb), self);
  gtk_box_pack_start(GTK_BOX(hbox), button_filter_list_rem, FALSE, FALSE, 2);
  gtk_widget_show(button_filter_list_rem);

  button_filter_list_add = gtk_button_new_with_label(_("Add"));
  g_signal_connect(button_filter_list_add, "clicked", G_CALLBACK(filter_list_add_clicked_cb), self);
  gtk_box_pack_start(GTK_BOX(hbox), button_filter_list_add, FALSE, FALSE, 2);
  gtk_widget_show(button_filter_list_add);

  self->filter_list_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), self->filter_list_entry, TRUE, TRUE, 0);
  gtk_widget_show(self->filter_list_entry);

  entry_completion = gtk_entry_completion_new();
  entry_list = gtk_list_store_new(1, G_TYPE_STRING);
  gtk_entry_completion_set_model(entry_completion, GTK_TREE_MODEL(entry_list));
  gtk_entry_completion_set_text_column(entry_completion, 0);
  gtk_entry_completion_set_minimum_key_length(entry_completion, 0);
  gtk_entry_set_completion(GTK_ENTRY(self->filter_list_entry), entry_completion);
  /* When we focus the entry, emit the changed signal so we get the hints immediately */
  /* also update the filter list hints from gsettings */
  g_signal_connect(self->filter_list_entry, "focus-in-event", G_CALLBACK(filter_list_entry_focus_in_cb), self);
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

  g_set_application_name(_("Indicator Notifications Settings"));

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

  bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  self = indicator_notifications_settings_new();

  status = g_application_run(G_APPLICATION(self), argc, argv);

  g_object_unref(self);

  return status;
}
