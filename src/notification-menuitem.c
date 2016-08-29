/*
 * notification-menuitem.h - A menuitem to display notifications.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include "notification-menuitem.h"
#include "urlregex.h"

#define NOTIFICATION_MENUITEM_MAX_CHARS 42
#define NOTIFICATION_MENUITEM_CLOSE_SELECT "indicator-notification-close-select"
#define NOTIFICATION_MENUITEM_CLOSE_DESELECT "indicator-notification-close-deselect"

enum {
  CLICKED,
  LAST_SIGNAL
};

static void notification_menuitem_class_init(NotificationMenuItemClass *klass);
static void notification_menuitem_init(NotificationMenuItem *self);

static void     notification_menuitem_activate(GtkMenuItem *menuitem);
static gboolean notification_menuitem_motion(GtkWidget *widget, GdkEventMotion *event);
static gboolean notification_menuitem_leave(GtkWidget *widget, GdkEventCrossing *event);
static gboolean notification_menuitem_button_press(GtkWidget *widget, GdkEventButton *event);
static gboolean notification_menuitem_button_release(GtkWidget *widget, GdkEventButton *event);
static void     notification_menuitem_select(GtkMenuItem *item);
static void     notification_menuitem_deselect(GtkMenuItem *item);

static gboolean notification_menuitem_activate_link_cb(GtkLabel *label, gchar *uri, gpointer user_data);
static gchar   *notification_menuitem_markup_body(const gchar *body);

static gboolean widget_contains_event(GtkWidget *widget, GdkEventButton *event);

static guint notification_menuitem_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (NotificationMenuItem, notification_menuitem, GTK_TYPE_MENU_ITEM);

static void
notification_menuitem_class_init(NotificationMenuItemClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
  GtkMenuItemClass *menu_item_class = GTK_MENU_ITEM_CLASS(klass);

  widget_class->leave_notify_event = notification_menuitem_leave;
  widget_class->motion_notify_event = notification_menuitem_motion;
  widget_class->button_press_event = notification_menuitem_button_press;
  widget_class->button_release_event = notification_menuitem_button_release;

  g_type_class_add_private(klass, sizeof(NotificationMenuItemPrivate));

  menu_item_class->hide_on_activate = FALSE;
  menu_item_class->activate = notification_menuitem_activate;
  menu_item_class->select = notification_menuitem_select;
  menu_item_class->deselect = notification_menuitem_deselect;

  /* Compile the urlregex patterns */
  urlregex_init();

  notification_menuitem_signals[CLICKED] =
    g_signal_new(NOTIFICATION_MENUITEM_SIGNAL_CLICKED,
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(NotificationMenuItemClass, clicked),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__UINT,
                 G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
notification_menuitem_init(NotificationMenuItem *self)
{
  self->priv = NOTIFICATION_MENUITEM_GET_PRIVATE(self);

  self->priv->pressed_close_image = FALSE;

  self->priv->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  self->priv->label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(self->priv->label), 0, 0);
  gtk_label_set_use_markup(GTK_LABEL(self->priv->label), TRUE);
  gtk_label_set_line_wrap(GTK_LABEL(self->priv->label), TRUE);
  gtk_label_set_line_wrap_mode(GTK_LABEL(self->priv->label), PANGO_WRAP_WORD_CHAR);
  gtk_label_set_max_width_chars(GTK_LABEL(self->priv->label), NOTIFICATION_MENUITEM_MAX_CHARS);
  gtk_label_set_track_visited_links(GTK_LABEL(self->priv->label), TRUE);

  g_signal_connect(self->priv->label, "activate-link", G_CALLBACK(notification_menuitem_activate_link_cb), self);

  gtk_box_pack_start(GTK_BOX(self->priv->hbox), self->priv->label, TRUE, TRUE, 0);
  gtk_widget_show(self->priv->label);

  self->priv->close_image = gtk_image_new_from_icon_name(NOTIFICATION_MENUITEM_CLOSE_DESELECT, GTK_ICON_SIZE_MENU);
  gtk_widget_show(self->priv->close_image);
  gtk_box_pack_start(GTK_BOX(self->priv->hbox), self->priv->close_image, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(self), self->priv->hbox);
  gtk_widget_show(self->priv->hbox);
}

GtkWidget * 
notification_menuitem_new(void)
{
  return g_object_new(NOTIFICATION_MENUITEM_TYPE, NULL);
}

/**
 * notification_menuitem_set_from_notification:
 * @self - the notification menuitem
 * @note - the notification object
 *
 * Sets the markup in the notification menuitem to display information about
 * the notification, as well as marking any links within the message body.
 **/
void
notification_menuitem_set_from_notification(NotificationMenuItem *self, Notification *note)
{
  g_return_if_fail(IS_NOTIFICATION(note));
  gchar *unescaped_timestamp_string = notification_timestamp_for_locale(note);

  gchar *app_name = g_markup_escape_text(notification_get_app_name(note), -1);
  gchar *summary = g_markup_escape_text(notification_get_summary(note), -1);
  gchar *body = notification_menuitem_markup_body(notification_get_body(note));
  gchar *timestamp_string = g_markup_escape_text(unescaped_timestamp_string, -1);

  gchar *markup = g_strdup_printf("<b>%s</b>\n%s\n<small><i>%s %s <b>%s</b></i></small>",
      summary, body, timestamp_string, _("from"), app_name);

  g_free(app_name);
  g_free(summary);
  g_free(body);
  g_free(unescaped_timestamp_string);
  g_free(timestamp_string);

  gtk_label_set_markup(GTK_LABEL(self->priv->label), markup);

  g_free(markup);
}

/**
 * notification_menuitem_activate:
 * @menuitem: the menuitem
 *
 * Emit a clicked event for the case where a keyboard activates a menuitem.
 **/
static void
notification_menuitem_activate(GtkMenuItem *menuitem)
{
  g_return_if_fail(IS_NOTIFICATION_MENUITEM(menuitem));

  g_signal_emit(NOTIFICATION_MENUITEM(menuitem), notification_menuitem_signals[CLICKED], 0);
}

/**
 * notification_menuitem_leave:
 * @widget - the widget
 * @event - the event
 *
 * Handle the leave-notify-event, by simply passing it on to the GtkLabel of
 * this menuitem.
 **/
static gboolean
notification_menuitem_leave(GtkWidget *widget, GdkEventCrossing *event)
{
  g_return_val_if_fail(IS_NOTIFICATION_MENUITEM(widget), FALSE);

  NotificationMenuItem *self = NOTIFICATION_MENUITEM(widget);

  gtk_widget_event(self->priv->label, (GdkEvent *)event);
  return FALSE;
}

/**
 * notification_menuitem_motion:
 * @widget - the widget
 * @event - the event
 *
 * Handle the motion-notify-event. It is passed on to the GtkLabel, but the
 * event (x, y) is mapped to the whole menuitem not the label so we have to
 * shift it over a bit into the label's allocation.
 **/
static gboolean
notification_menuitem_motion(GtkWidget *widget, GdkEventMotion *event)
{
  g_return_val_if_fail(IS_NOTIFICATION_MENUITEM(widget), FALSE);

  NotificationMenuItem *self = NOTIFICATION_MENUITEM(widget);

  GtkAllocation self_alloc;
  GtkAllocation label_alloc;

  gtk_widget_get_allocation(GTK_WIDGET(self), &self_alloc);
  gtk_widget_get_allocation(self->priv->label, &label_alloc);

  /* The event is mapped to the menu item's allocation, so we need to shift it
   * to the label's allocation so that links are probably selected.
   */
  GdkEventMotion *e = (GdkEventMotion *)gdk_event_copy((GdkEvent *)event);
  e->x = event->x - (label_alloc.x - self_alloc.x);
  e->y = event->y - (label_alloc.y - self_alloc.y);

  gtk_widget_event(self->priv->label, (GdkEvent *)e);

  gdk_event_free((GdkEvent *)e);
  return FALSE;
}

/**
 * notification_menuitem_button_press:
 * @widget: the menuitem
 * @event: the button press event
 *
 * Override the menuitem button-press-event.
 **/
static gboolean
notification_menuitem_button_press(GtkWidget *widget, GdkEventButton *event)
{
  g_return_val_if_fail(IS_NOTIFICATION_MENUITEM(widget), FALSE);

  NotificationMenuItem *self = NOTIFICATION_MENUITEM(widget);

  /* The context menu breaks everything so disable it for now */
  if (event->button == GDK_BUTTON_PRIMARY && widget_contains_event(self->priv->label, event)) {
    gtk_widget_event(self->priv->label, (GdkEvent *)event);
  }
  else if (widget_contains_event(self->priv->close_image, event)) {
    self->priv->pressed_close_image = TRUE;
  }
  return TRUE;
}

/**
 * notification_menuitem_button_release:
 * @widget: the menuitem
 * @event: the button release event
 *
 * Override the menuitem button-release-event so that the menu isn't hidden
 * when the item is removed. Also the event is passed on to the label so we can
 * get an activate-link signal when a link is clicked.
 **/
static gboolean
notification_menuitem_button_release(GtkWidget *widget, GdkEventButton *event)
{
  g_return_val_if_fail(IS_NOTIFICATION_MENUITEM(widget), FALSE);

  NotificationMenuItem *self = NOTIFICATION_MENUITEM(widget);

  if (widget_contains_event(self->priv->close_image, event)) {
    if (self->priv->pressed_close_image)
      g_signal_emit(NOTIFICATION_MENUITEM(widget), notification_menuitem_signals[CLICKED], 0, event->button);
  }
  else {
    /* The context menu breaks everything so disable it for now */
    if (event->button == GDK_BUTTON_PRIMARY) {
      gtk_widget_event(self->priv->label, (GdkEvent *)event);
    }
  }
  self->priv->pressed_close_image = FALSE;
  return TRUE;
}

/**
 * notification_menuitem_select:
 * @menuitem - the menuitem
 *
 * Handle the menuitem select signal. We don't want to set PRELIGHT on the
 * menuitem because in various themes it becomes very hard to see the links.
 * Instead we set a special close image to show that the notification is
 * selected for keyboard navigation.
 **/
static void
notification_menuitem_select(GtkMenuItem *menuitem)
{
  g_return_if_fail(IS_NOTIFICATION_MENUITEM(menuitem));

  NotificationMenuItem *self = NOTIFICATION_MENUITEM(menuitem);

  gtk_image_set_from_icon_name(GTK_IMAGE(self->priv->close_image),
      NOTIFICATION_MENUITEM_CLOSE_SELECT,
      GTK_ICON_SIZE_MENU);
}

/**
 * notification_menuitem_deselect:
 * @menuitem - the menuitem
 *
 * Same as notification_menuitem_select, but sets the opposite close image.
 **/
static void
notification_menuitem_deselect(GtkMenuItem *menuitem)
{
  g_return_if_fail(IS_NOTIFICATION_MENUITEM(menuitem));

  NotificationMenuItem *self = NOTIFICATION_MENUITEM(menuitem);

  gtk_image_set_from_icon_name(GTK_IMAGE(self->priv->close_image),
      NOTIFICATION_MENUITEM_CLOSE_DESELECT,
      GTK_ICON_SIZE_MENU);
}

/**
 * notification_menuitem_activate_link_cb:
 * @label - the label
 * @uri - the link that was activated
 * @user_data - the notification menuitem
 *
 * We override the activate-link signal of the GtkLabel because we need to
 * deactivate the menu shell when it is clicked, otherwise it stays stuck to
 * the screen as the browser loads.
 **/
static gboolean
notification_menuitem_activate_link_cb(GtkLabel *label, gchar *uri, gpointer user_data)
{
  g_return_val_if_fail(IS_NOTIFICATION_MENUITEM(user_data), FALSE);

  NotificationMenuItem *self = NOTIFICATION_MENUITEM(user_data);

  /* Show the link */
  GError *error = NULL;

  if (!gtk_show_uri(gtk_widget_get_screen(GTK_WIDGET(label)),
          uri, gtk_get_current_event_time(), &error)) {
    g_warning("Unable to show '%s': %s", uri, error->message);
    g_error_free(error);
  }

  /* Deactivate the menu shell so it doesn't block the screen */
  GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(self));
  if (GTK_IS_MENU_SHELL(parent)) {
    gtk_menu_shell_deactivate(GTK_MENU_SHELL(parent));
  }

  return TRUE;
}

/**
 * notification_menuitem_markup_body:
 * @body - the body of a notification
 *
 * Scans through the body text escaping everything that isn't a link. The links
 * are marked up as anchors with hrefs.
 **/
static gchar *
notification_menuitem_markup_body(const gchar *body)
{
  GList *list = urlregex_split_all(body);
  guint len = g_list_length(list);
  gchar **str_array = g_new0(gchar *, len + 1);
  guint i = 0;
  GList *item;
  gchar *escaped_text;
  gchar *escaped_expanded;

  for (item = list; item; item = item->next, i++) {
    MatchGroup *group = (MatchGroup *)item->data;
    if (group->type == MATCHED) {
      escaped_text = g_markup_escape_text(group->text, -1);
      escaped_expanded = g_markup_escape_text(group->expanded, -1);
      str_array[i] = g_strdup_printf("<a href=\"%s\">%s</a>", escaped_expanded, escaped_text);
      g_free(escaped_text);
      g_free(escaped_expanded);
    }
    else {
      str_array[i] = g_markup_escape_text(group->text, -1);
    }
  }

  urlregex_matchgroup_list_free(list);
  gchar *result = g_strjoinv(NULL, str_array);
  g_strfreev(str_array);
  return result;
}

/**
 * widget_contains_event:
 * @widget - the widget
 * @event - the event
 *
 * Determines whether the (x, y) coordinates of the event fall inside the
 * widget.
 **/
static gboolean
widget_contains_event(GtkWidget *widget, GdkEventButton *event)
{
  if (gtk_widget_get_window(widget) == NULL)
    return FALSE;

  GtkAllocation allocation;

  gtk_widget_get_allocation(widget, &allocation);

  GdkWindow *window = gtk_widget_get_window(widget);

  int xwin, ywin;

  gdk_window_get_origin(window, &xwin, &ywin);

  int xmin = allocation.x;
  int xmax = allocation.x + allocation.width;
  int ymin = allocation.y;
  int ymax = allocation.y + allocation.height; 
  int x = event->x_root - xwin;
  int y = event->y_root - ywin;

  return x >= xmin && x <= xmax && y >= ymin && y <= ymax;
}
