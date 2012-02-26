/*
 * notification-menuitem.h - A menuitem to display notifications.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include "notification-menuitem.h"

#define NOTIFICATION_MENUITEM_MAX_CHARS 42

static void notification_menuitem_class_init(NotificationMenuItemClass *klass);
static void notification_menuitem_init(NotificationMenuItem *self);

G_DEFINE_TYPE (NotificationMenuItem, notification_menuitem, GTK_TYPE_MENU_ITEM);

static void
notification_menuitem_class_init(NotificationMenuItemClass *klass)
{
  GtkMenuItemClass *menu_item_class = GTK_MENU_ITEM_CLASS(klass);

  g_type_class_add_private(klass, sizeof(NotificationMenuItemPrivate));

  menu_item_class->hide_on_activate = FALSE;
}

static void
notification_menuitem_init(NotificationMenuItem *self)
{
  self->priv = NOTIFICATION_MENUITEM_GET_PRIVATE(self);

  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  self->priv->label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(self->priv->label), 0, 0);
  gtk_label_set_use_markup(GTK_LABEL(self->priv->label), TRUE);
  gtk_label_set_line_wrap(GTK_LABEL(self->priv->label), TRUE);
  gtk_label_set_line_wrap_mode(GTK_LABEL(self->priv->label), PANGO_WRAP_WORD_CHAR);
  gtk_label_set_max_width_chars(GTK_LABEL(self->priv->label), NOTIFICATION_MENUITEM_MAX_CHARS);

  gtk_box_pack_start(GTK_BOX(hbox), self->priv->label, TRUE, TRUE, 0);
  gtk_widget_show(self->priv->label);

  self->priv->close_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
  gtk_widget_show(self->priv->close_image);
  gtk_box_pack_start(GTK_BOX(hbox), self->priv->close_image, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(self), hbox);
  gtk_widget_show(hbox);
}

GtkWidget * 
notification_menuitem_new(void)
{
  return g_object_new(NOTIFICATION_MENUITEM_TYPE, NULL);
}

void
notification_menuitem_set_from_notification(NotificationMenuItem *self, Notification *note)
{
  g_return_if_fail(IS_NOTIFICATION(note));
  gchar *unescaped_timestamp_string = notification_timestamp_for_locale(note);

  gchar *app_name = g_markup_escape_text(notification_get_app_name(note), -1);
  gchar *summary = g_markup_escape_text(notification_get_summary(note), -1);
  gchar *body = g_markup_escape_text(notification_get_body(note), -1);
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
