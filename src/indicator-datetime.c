/*
An indicator to time and date related information in the menubar.

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

/* Indicator Stuff */
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>

/* DBusMenu */
#include <libdbusmenu-gtk/menu.h>
#include <libido/libido.h>
#include <libdbusmenu-gtk/menuitem.h>

#include "utils.h"
#include "dbus-shared.h"
#include "settings-shared.h"


#define INDICATOR_DATETIME_TYPE            (indicator_datetime_get_type ())
#define INDICATOR_DATETIME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_DATETIME_TYPE, IndicatorDatetime))
#define INDICATOR_DATETIME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_DATETIME_TYPE, IndicatorDatetimeClass))
#define IS_INDICATOR_DATETIME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_DATETIME_TYPE))
#define IS_INDICATOR_DATETIME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_DATETIME_TYPE))
#define INDICATOR_DATETIME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_DATETIME_TYPE, IndicatorDatetimeClass))

typedef struct _IndicatorDatetime         IndicatorDatetime;
typedef struct _IndicatorDatetimeClass    IndicatorDatetimeClass;
typedef struct _IndicatorDatetimePrivate  IndicatorDatetimePrivate;

struct _IndicatorDatetimeClass {
	IndicatorObjectClass parent_class;
};

struct _IndicatorDatetime {
	IndicatorObject parent;
	IndicatorDatetimePrivate * priv;
};

struct _IndicatorDatetimePrivate {
	GtkLabel * label;
	guint timer;

	gchar * time_string;

	gboolean show_clock;
	gint time_mode;
	gboolean show_seconds;
	gboolean show_date;
	gboolean show_day;
	gchar * custom_string;
	gboolean custom_show_seconds;

	gboolean show_week_numbers;
	gboolean show_calendar;
	gint week_start;
	
	guint idle_measure;
	gint  max_width;

	IndicatorServiceManager * sm;
	DbusmenuGtkMenu * menu;

	GCancellable * service_proxy_cancel;
	GDBusProxy * service_proxy;
	IdoCalendarMenuItem *ido_calendar;

	GList * timezone_items;

	GSettings * settings;

	GtkSizeGroup * indicator_right_group;
};

/* Enum for the properties so that they can be quickly
   found and looked up. */
enum {
	PROP_0,
	PROP_SHOW_CLOCK,
	PROP_TIME_FORMAT,
	PROP_SHOW_SECONDS,
	PROP_SHOW_DAY,
	PROP_SHOW_DATE,
	PROP_CUSTOM_TIME_FORMAT,
	PROP_SHOW_WEEK_NUMBERS,
	PROP_SHOW_CALENDAR
};

typedef struct _indicator_item_t indicator_item_t;
struct _indicator_item_t {
	IndicatorDatetime * self;
	DbusmenuMenuitem * mi;
	GtkWidget * gmi;
	GtkWidget * icon;
	GtkWidget * label;
	GtkWidget * right;
};

#define PROP_SHOW_CLOCK_S               "show-clock"
#define PROP_TIME_FORMAT_S              "time-format"
#define PROP_SHOW_SECONDS_S             "show-seconds"
#define PROP_SHOW_DAY_S                 "show-day"
#define PROP_SHOW_DATE_S                "show-date"
#define PROP_CUSTOM_TIME_FORMAT_S       "custom-time-format"
#define PROP_SHOW_WEEK_NUMBERS_S        "show-week-numbers"
#define PROP_SHOW_CALENDAR_S            "show-calendar"

#define INDICATOR_DATETIME_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_DATETIME_TYPE, IndicatorDatetimePrivate))

enum {
	STRFTIME_MASK_NONE    = 0,      /* Hours or minutes as we always test those */
	STRFTIME_MASK_SECONDS = 1 << 0, /* Seconds count */
	STRFTIME_MASK_AMPM    = 1 << 1, /* AM/PM counts */
	STRFTIME_MASK_WEEK    = 1 << 2, /* Day of the week maters (Sat, Sun, etc.) */
	STRFTIME_MASK_DAY     = 1 << 3, /* Day of the month counts (Feb 1st) */
	STRFTIME_MASK_MONTH   = 1 << 4, /* Which month matters */
	STRFTIME_MASK_YEAR    = 1 << 5, /* Which year matters */
	/* Last entry, combines all previous */
	STRFTIME_MASK_ALL     = (STRFTIME_MASK_SECONDS | STRFTIME_MASK_AMPM | STRFTIME_MASK_WEEK | STRFTIME_MASK_DAY | STRFTIME_MASK_MONTH | STRFTIME_MASK_YEAR)
};

GType indicator_datetime_get_type (void);

static void indicator_datetime_class_init (IndicatorDatetimeClass *klass);
static void indicator_datetime_init       (IndicatorDatetime *self);
static void set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void indicator_datetime_dispose    (GObject *object);
static void indicator_datetime_finalize   (GObject *object);
static GtkLabel * get_label               (IndicatorObject * io);
static GtkMenu *  get_menu                (IndicatorObject * io);
static const gchar * get_accessible_desc  (IndicatorObject * io);
static GVariant * bind_enum_set           (const GValue * value, const GVariantType * type, gpointer user_data);
static gboolean bind_enum_get             (GValue * value, GVariant * variant, gpointer user_data);
static gchar * generate_format_string_now (IndicatorDatetime * self);
static void update_label                  (IndicatorDatetime * io, GDateTime ** datetime);
static void guess_label_size              (IndicatorDatetime * self);
static void setup_timer                   (IndicatorDatetime * self, GDateTime * datetime);
static void update_time                   (IndicatorDatetime * self);
static void receive_signal                (GDBusProxy * proxy, gchar * sender_name, gchar * signal_name, GVariant * parameters, gpointer user_data);
static void service_proxy_cb (GObject * object, GAsyncResult * res, gpointer user_data);
static gint generate_strftime_bitmask     (const char *time_str);
static void timezone_update_labels        (indicator_item_t * mi_data);
static gboolean new_calendar_item         (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient   * client, gpointer user_data);
static gboolean new_appointment_item      (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client, gpointer user_data);
static gboolean new_timezone_item         (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient   * client, gpointer user_data);

/* Indicator Module Config */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_DATETIME_TYPE)

G_DEFINE_TYPE (IndicatorDatetime, indicator_datetime, INDICATOR_OBJECT_TYPE);

static void
indicator_datetime_class_init (IndicatorDatetimeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (IndicatorDatetimePrivate));

	object_class->dispose = indicator_datetime_dispose;
	object_class->finalize = indicator_datetime_finalize;

	object_class->set_property = set_property;
	object_class->get_property = get_property;

	IndicatorObjectClass * io_class = INDICATOR_OBJECT_CLASS(klass);

	io_class->get_label = get_label;
	io_class->get_menu  = get_menu;
	io_class->get_accessible_desc = get_accessible_desc;

	g_object_class_install_property (object_class,
	                                 PROP_SHOW_CLOCK,
	                                 g_param_spec_boolean(PROP_SHOW_CLOCK_S,
	                                                      "Whether to show the clock in the menu bar.",
	                                                      "Shows indicator-datetime in the shell's menu bar.",
	                                                      TRUE, /* default */
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_TIME_FORMAT,
	                                 g_param_spec_int(PROP_TIME_FORMAT_S,
	                                                  "A choice of which format should be used on the panel",
	                                                  "Chooses between letting the locale choose the time, 12-hour time, 24-time or using the custom string passed to strftime().",
	                                                  SETTINGS_TIME_LOCALE, /* min */
	                                                  SETTINGS_TIME_CUSTOM, /* max */
	                                                  SETTINGS_TIME_LOCALE, /* default */
	                                                  G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_SHOW_SECONDS,
	                                 g_param_spec_boolean(PROP_SHOW_SECONDS_S,
	                                                      "Whether to show seconds in the indicator.",
	                                                      "Shows seconds along with the time in the indicator.  Also effects refresh interval.",
	                                                      FALSE, /* default */
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_SHOW_DAY,
	                                 g_param_spec_boolean(PROP_SHOW_DAY_S,
	                                                      "Whether to show the day of the week in the indicator.",
	                                                      "Shows the day of the week along with the time in the indicator.",
	                                                      FALSE, /* default */
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_SHOW_DATE,
	                                 g_param_spec_boolean(PROP_SHOW_DATE_S,
	                                                      "Whether to show the day and month in the indicator.",
	                                                      "Shows the day and month along with the time in the indicator.",
	                                                      FALSE, /* default */
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_CUSTOM_TIME_FORMAT,
	                                 g_param_spec_string(PROP_CUSTOM_TIME_FORMAT_S,
	                                                     "The format that is used to show the time on the panel.",
	                                                     "A format string in the form used to pass to strftime to make a string for displaying on the panel.",
	                                                     DEFAULT_TIME_FORMAT,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
	                                 PROP_SHOW_WEEK_NUMBERS,
	                                 g_param_spec_boolean(PROP_SHOW_WEEK_NUMBERS_S,
	                                                      "Whether to show the week numbers in the calendar.",
	                                                      "Shows the week numbers in the monthly calendar in indicator-datetime's menu.",
	                                                      FALSE, /* default */
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
	                                 PROP_SHOW_CALENDAR,
	                                 g_param_spec_boolean(PROP_SHOW_CALENDAR_S,
	                                                      "Whether to show the calendar.",
	                                                      "Shows the monthly calendar in indicator-datetime's menu.",
	                                                      TRUE, /* default */
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	return;
}

static void
menu_visible_notfy_cb(GtkWidget * menu, G_GNUC_UNUSED GParamSpec *pspec, gpointer user_data)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(user_data);
	g_debug("notify visible signal recieved");
	
	// we should only react if we're currently visible
	gboolean visible;
	g_object_get(G_OBJECT(menu), "visible", &visible, NULL);
	if (visible) return;
	g_debug("notify visible menu hidden, resetting date");
	
	time_t curtime;
	
	time(&curtime);
  	struct tm *today = localtime(&curtime);
  	int y = today->tm_year;
  	int m = today->tm_mon;
  	int d = today->tm_mday;
  	
  	// Set the calendar to todays date
	ido_calendar_menu_item_set_date (self->priv->ido_calendar, y+1900, m, d);
	
	// Make sure the day-selected signal is sent so the menu updates - may duplicate
	/*GVariant *variant = g_variant_new_uint32((guint)curtime);
	guint timestamp = (guint)time(NULL);
	dbusmenu_menuitem_handle_event(DBUSMENU_MENUITEM(self->priv->ido_calendar), "day-selected", variant, timestamp);*/
}

static void
indicator_datetime_init (IndicatorDatetime *self)
{
	self->priv = INDICATOR_DATETIME_GET_PRIVATE(self);

	self->priv->label = NULL;
	self->priv->timer = 0;

	self->priv->idle_measure = 0;
	self->priv->max_width = 0;

	self->priv->show_clock = TRUE;
	self->priv->time_mode = SETTINGS_TIME_LOCALE;
	self->priv->show_seconds = FALSE;
	self->priv->show_date = FALSE;
	self->priv->show_day = FALSE;
	self->priv->custom_string = g_strdup(DEFAULT_TIME_FORMAT);
	self->priv->custom_show_seconds = FALSE;

	self->priv->time_string = generate_format_string_now(self);

	self->priv->service_proxy = NULL;

	self->priv->sm = NULL;
	self->priv->menu = NULL;

	self->priv->settings = g_settings_new(SETTINGS_INTERFACE);
	if (self->priv->settings != NULL) {
		g_settings_bind(self->priv->settings,
		                SETTINGS_SHOW_CLOCK_S,
		                self,
		                PROP_SHOW_CLOCK_S,
		                G_SETTINGS_BIND_DEFAULT);
		g_settings_bind_with_mapping(self->priv->settings,
		                SETTINGS_TIME_FORMAT_S,
		                self,
		                PROP_TIME_FORMAT_S,
		                G_SETTINGS_BIND_DEFAULT,
		                bind_enum_get,
		                bind_enum_set,
		                NULL, NULL); /* Userdata and destroy func */
		g_settings_bind(self->priv->settings,
		                SETTINGS_SHOW_SECONDS_S,
		                self,
		                PROP_SHOW_SECONDS_S,
		                G_SETTINGS_BIND_DEFAULT);
		g_settings_bind(self->priv->settings,
		                SETTINGS_SHOW_DAY_S,
		                self,
		                PROP_SHOW_DAY_S,
		                G_SETTINGS_BIND_DEFAULT);
		g_settings_bind(self->priv->settings,
		                SETTINGS_SHOW_DATE_S,
		                self,
		                PROP_SHOW_DATE_S,
		                G_SETTINGS_BIND_DEFAULT);
		g_settings_bind(self->priv->settings,
		                SETTINGS_CUSTOM_TIME_FORMAT_S,
		                self,
		                PROP_CUSTOM_TIME_FORMAT_S,
		                G_SETTINGS_BIND_DEFAULT);
		g_settings_bind(self->priv->settings,
		                SETTINGS_SHOW_WEEK_NUMBERS_S,
		                self,
		                PROP_SHOW_WEEK_NUMBERS_S,
		                G_SETTINGS_BIND_DEFAULT);
		g_settings_bind(self->priv->settings,
		                SETTINGS_SHOW_CALENDAR_S,
		                self,
		                PROP_SHOW_CALENDAR_S,
		                G_SETTINGS_BIND_DEFAULT);
	} else {
		g_warning("Unable to get settings for '" SETTINGS_INTERFACE "'");
	}

	self->priv->sm = indicator_service_manager_new_version(SERVICE_NAME, SERVICE_VERSION);
	self->priv->indicator_right_group = GTK_SIZE_GROUP(gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL));

	self->priv->menu = dbusmenu_gtkmenu_new(SERVICE_NAME, MENU_OBJ);

	g_signal_connect(self->priv->menu, "notify::visible", G_CALLBACK(menu_visible_notfy_cb), self);
	
	DbusmenuGtkClient *client = dbusmenu_gtkmenu_get_client(self->priv->menu);
	dbusmenu_client_add_type_handler_full(DBUSMENU_CLIENT(client), DBUSMENU_CALENDAR_MENUITEM_TYPE, new_calendar_item, self, NULL);
	dbusmenu_client_add_type_handler_full(DBUSMENU_CLIENT(client), APPOINTMENT_MENUITEM_TYPE, new_appointment_item, self, NULL);
	dbusmenu_client_add_type_handler_full(DBUSMENU_CLIENT(client), TIMEZONE_MENUITEM_TYPE, new_timezone_item, self, NULL);

	self->priv->service_proxy_cancel = g_cancellable_new();

	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
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
service_proxy_cb (GObject * object, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;

	IndicatorDatetime * self = INDICATOR_DATETIME(user_data);
	g_return_if_fail(self != NULL);

	GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &error);

	IndicatorDatetimePrivate * priv = INDICATOR_DATETIME_GET_PRIVATE(self);

	if (priv->service_proxy_cancel != NULL) {
		g_object_unref(priv->service_proxy_cancel);
		priv->service_proxy_cancel = NULL;
	}

	if (error != NULL) {
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
indicator_datetime_dispose (GObject *object)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(object);

	if (self->priv->label != NULL) {
		g_object_unref(self->priv->label);
		self->priv->label = NULL;
	}

	if (self->priv->timer != 0) {
		g_source_remove(self->priv->timer);
		self->priv->timer = 0;
	}

	if (self->priv->idle_measure != 0) {
		g_source_remove(self->priv->idle_measure);
		self->priv->idle_measure = 0;
	}

	if (self->priv->menu != NULL) {
		g_object_unref(G_OBJECT(self->priv->menu));
		self->priv->menu = NULL;
	}

	if (self->priv->sm != NULL) {
		g_object_unref(G_OBJECT(self->priv->sm));
		self->priv->sm = NULL;
	}

	if (self->priv->settings != NULL) {
		g_object_unref(G_OBJECT(self->priv->settings));
		self->priv->settings = NULL;
	}

	if (self->priv->service_proxy != NULL) {
		g_object_unref(self->priv->service_proxy);
		self->priv->service_proxy = NULL;
	}

	if (self->priv->indicator_right_group != NULL) {
		g_object_unref(G_OBJECT(self->priv->indicator_right_group));
		self->priv->indicator_right_group = NULL;
	}

	G_OBJECT_CLASS (indicator_datetime_parent_class)->dispose (object);
	return;
}

static void
indicator_datetime_finalize (GObject *object)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(object);

	if (self->priv->time_string != NULL) {
		g_free(self->priv->time_string);
		self->priv->time_string = NULL;
	}

	if (self->priv->custom_string != NULL) {
		g_free(self->priv->custom_string);
		self->priv->custom_string = NULL;
	}

	G_OBJECT_CLASS (indicator_datetime_parent_class)->finalize (object);
	return;
}

/* Turns the int value into a string GVariant */
static GVariant *
bind_enum_set (const GValue * value, const GVariantType * type, gpointer user_data)
{
	switch (g_value_get_int(value)) {
	case SETTINGS_TIME_LOCALE:
		return g_variant_new_string("locale-default");
	case SETTINGS_TIME_12_HOUR:
		return g_variant_new_string("12-hour");
	case SETTINGS_TIME_24_HOUR:
		return g_variant_new_string("24-hour");
	case SETTINGS_TIME_CUSTOM:
		return g_variant_new_string("custom");
	default:
		return NULL;
	}
}

/* Turns a string GVariant into an int value */
static gboolean
bind_enum_get (GValue * value, GVariant * variant, gpointer user_data)
{
	const gchar * str = g_variant_get_string(variant, NULL);
	gint output = 0;

	if (g_strcmp0(str, "locale-default") == 0) {
		output = SETTINGS_TIME_LOCALE;
	} else if (g_strcmp0(str, "12-hour") == 0) {
		output = SETTINGS_TIME_12_HOUR;
	} else if (g_strcmp0(str, "24-hour") == 0) {
		output = SETTINGS_TIME_24_HOUR;
	} else if (g_strcmp0(str, "custom") == 0) {
		output = SETTINGS_TIME_CUSTOM;
	} else {
		return FALSE;
	}

	g_value_set_int(value, output);
	return TRUE;
}

static void
timezone_update_all_labels (IndicatorDatetime * self)
{
	IndicatorDatetimePrivate *priv = INDICATOR_DATETIME_GET_PRIVATE(self);
	g_list_foreach(priv->timezone_items, (GFunc)timezone_update_labels, NULL);
}

/* Sets a property on the object */
static void
set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(object);
	gboolean update = FALSE;

	switch(prop_id) {
	case PROP_SHOW_CLOCK: {
		if (g_value_get_boolean(value) != self->priv->show_clock) {
			self->priv->show_clock = g_value_get_boolean(value);
			if (self->priv->label != NULL) {
				gtk_widget_set_visible (GTK_WIDGET (self->priv->label), self->priv->show_clock);
			}
		}
		break;
	}
	case PROP_TIME_FORMAT: {
		gint newval = g_value_get_int(value);
		if (newval != self->priv->time_mode) {
			update = TRUE;
			self->priv->time_mode = newval;
			setup_timer(self, NULL);			
		}
		break;
	}
	case PROP_SHOW_SECONDS: {
		if (g_value_get_boolean(value) != self->priv->show_seconds) {
			self->priv->show_seconds = !self->priv->show_seconds;
			if (self->priv->time_mode != SETTINGS_TIME_CUSTOM) {
				update = TRUE;
				setup_timer(self, NULL);
			}
		}
		break;
	}
	case PROP_SHOW_DAY: {
		if (g_value_get_boolean(value) != self->priv->show_day) {
			self->priv->show_day = !self->priv->show_day;
			if (self->priv->time_mode != SETTINGS_TIME_CUSTOM) {
				update = TRUE;
			}
		}
		break;
	}
	case PROP_SHOW_DATE: {
		if (g_value_get_boolean(value) != self->priv->show_date) {
			self->priv->show_date = !self->priv->show_date;
			if (self->priv->time_mode != SETTINGS_TIME_CUSTOM) {
				update = TRUE;
			}
		}
		break;
	}
	case PROP_CUSTOM_TIME_FORMAT: {
		const gchar * newstr = g_value_get_string(value);
		if (g_strcmp0(newstr, self->priv->custom_string) != 0) {
			if (self->priv->custom_string != NULL) {
				g_free(self->priv->custom_string);
				self->priv->custom_string = NULL;
			}
			self->priv->custom_string = g_strdup(newstr);
			gint time_mask = generate_strftime_bitmask(newstr);
			self->priv->custom_show_seconds = (time_mask & STRFTIME_MASK_SECONDS);
			if (self->priv->time_mode == SETTINGS_TIME_CUSTOM) {
				update = TRUE;
				setup_timer(self, NULL);
			}
		}
		break;
	}
	case PROP_SHOW_WEEK_NUMBERS: {
		if (g_value_get_boolean(value) != self->priv->show_week_numbers) {
			GtkCalendarDisplayOptions flags = ido_calendar_menu_item_get_display_options (self->priv->ido_calendar);
			if (g_value_get_boolean(value) == TRUE)
				flags |= GTK_CALENDAR_SHOW_WEEK_NUMBERS;
			else
				flags &= ~GTK_CALENDAR_SHOW_WEEK_NUMBERS;
			ido_calendar_menu_item_set_display_options (self->priv->ido_calendar, flags);
			self->priv->show_week_numbers = g_value_get_boolean(value);
		}
		break;
	}
	case PROP_SHOW_CALENDAR: {
		if (g_value_get_boolean(value) != self->priv->show_calendar) {
			self->priv->show_calendar = g_value_get_boolean(value);
			if (self->priv->ido_calendar != NULL) {
				gtk_widget_set_visible (GTK_WIDGET (self->priv->ido_calendar), self->priv->show_calendar);
			}
		}
		break;
	} 
	default: {
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		return;
	}
	}

	if (!update) {
		return;
	}

	/* Get the new format string */
	gchar * newformat = generate_format_string_now(self);

	/* check to ensure the format really changed */
	if (g_strcmp0(self->priv->time_string, newformat) == 0) {
		g_free(newformat);
		return;
	}

	/* Okay now process the change */
	if (self->priv->time_string != NULL) {
		g_free(self->priv->time_string);
		self->priv->time_string = NULL;
	}
	self->priv->time_string = newformat;

	/* And update everything */
	update_label(self, NULL);
	timezone_update_all_labels(self);
	guess_label_size(self);

	return;
}

/* Gets a property from the object */
static void
get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(object);

	switch(prop_id) {
	case PROP_SHOW_CLOCK:
		g_value_set_boolean(value, self->priv->show_clock);
		break;
	case PROP_TIME_FORMAT:
		g_value_set_int(value, self->priv->time_mode);
		break;
	case PROP_SHOW_SECONDS:
		g_value_set_boolean(value, self->priv->show_seconds);
		break;
	case PROP_SHOW_DAY:
		g_value_set_boolean(value, self->priv->show_day);
		break;
	case PROP_SHOW_DATE:
		g_value_set_boolean(value, self->priv->show_date);
		break;
	case PROP_CUSTOM_TIME_FORMAT:
		g_value_set_string(value, self->priv->custom_string);
		break;
	case PROP_SHOW_WEEK_NUMBERS:
		g_value_set_boolean(value, self->priv->show_week_numbers);
		break;
	case PROP_SHOW_CALENDAR:
		g_value_set_boolean(value, self->priv->show_calendar);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		return;
	}

	return;
}

/* Looks at the size of the label, if it grew beyond what we
   thought was the max, make sure it doesn't shrink again. */
static gboolean
idle_measure (gpointer data)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(data);
	self->priv->idle_measure = 0;

	GtkAllocation allocation;
	gtk_widget_get_allocation(GTK_WIDGET(self->priv->label), &allocation);

	if (allocation.width > self->priv->max_width) {
		if (self->priv->max_width != 0) {
			g_warning("Guessed wrong.  We thought the max would be %d but we're now at %d", self->priv->max_width, allocation.width);
		}
		self->priv->max_width = allocation.width;
		gtk_widget_set_size_request(GTK_WIDGET(self->priv->label), self->priv->max_width, -1);
	}

	return FALSE;
}

/* Updates the accessible description */
static void
update_accessible_description (IndicatorDatetime * io)
{
	GList * entries = indicator_object_get_entries(INDICATOR_OBJECT(io));
	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)entries->data;

	entry->accessible_desc = get_accessible_desc(INDICATOR_OBJECT(io));

	g_signal_emit(G_OBJECT(io),
	              INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE_ID,
	              0,
	              entry,
	              TRUE);

	g_list_free(entries);

	return;
}

/* Updates the label to be the current time. */
static void
set_label_to_time_in_zone (IndicatorDatetime * self, GtkLabel * label,
                           GTimeZone * tz, const gchar * format,
                           GDateTime ** datetime)
{
	GDateTime * datetime_now;
	if (tz == NULL)
		datetime_now = g_date_time_new_now_local();
	else
		datetime_now = g_date_time_new_now(tz);

	gchar * timestr;
	if (format == NULL) {
		gchar * format_for_time = generate_format_string_at_time(datetime_now);
		timestr = g_date_time_format(datetime_now, format_for_time);
		g_free(format_for_time);
	}
	else {
		timestr = g_date_time_format(datetime_now, format);
	}

	gboolean use_markup = FALSE;
	if (pango_parse_markup(timestr, -1, 0, NULL, NULL, NULL, NULL))
		use_markup = TRUE;

	if (use_markup)
		gtk_label_set_markup(label, timestr);
	else
		gtk_label_set_text(label, timestr);

	g_free(timestr);

	if (datetime)
		*datetime = datetime_now;
	else
		g_date_time_unref(datetime_now);

	return;
}

/* Updates the label to be the current time. */
static void
update_label (IndicatorDatetime * io, GDateTime ** datetime)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(io);

	if (self->priv->label == NULL) return;

	set_label_to_time_in_zone(self, self->priv->label, NULL, self->priv->time_string, datetime);

	if (self->priv->idle_measure == 0) {
		self->priv->idle_measure = g_idle_add(idle_measure, io);
	}

	update_accessible_description(io);

	return;
}

/* Update the time right now.  Usually the result of a timezone switch. */
static void
update_time (IndicatorDatetime * self)
{
	GDateTime * dt;
	update_label(self, &dt);
	timezone_update_all_labels(self);
	setup_timer(self, dt);
	g_date_time_unref(dt);
	return;
}

/* Receives all signals from the service, routed to the appropriate functions */
static void
receive_signal (GDBusProxy * proxy, gchar * sender_name, gchar * signal_name,
                GVariant * parameters, gpointer user_data)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(user_data);

	if (g_strcmp0(signal_name, "UpdateTime") == 0) {
		update_time(self);
	}

	return;
}

/* Runs every minute and updates the time */
gboolean
timer_func (gpointer user_data)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(user_data);
	self->priv->timer = 0;
	GDateTime * dt;
	update_label(self, &dt);
	timezone_update_all_labels(self);
	setup_timer(self, dt);
	g_date_time_unref(dt);
	return FALSE;
}

/* Configure the timer to run the next time through */
static void
setup_timer (IndicatorDatetime * self, GDateTime * datetime)
{
	gboolean unref = FALSE;

	if (self->priv->timer != 0) {
		g_source_remove(self->priv->timer);
		self->priv->timer = 0;
	}
	
	if (self->priv->show_seconds ||
		(self->priv->time_mode == SETTINGS_TIME_CUSTOM && self->priv->custom_show_seconds)) {
		self->priv->timer = g_timeout_add_full(G_PRIORITY_HIGH, 999, timer_func, self, NULL);
	} else {
		if (datetime == NULL) {
			datetime = g_date_time_new_now_local();
			unref = TRUE;
		}

		/* Plus 2 so we're just after the minute, don't want to be early. */
		gint seconds = (gint)g_date_time_get_seconds(datetime);
		self->priv->timer = g_timeout_add_seconds(60 - seconds + 2, timer_func, self);

		if (unref) {
			g_date_time_unref(datetime);
		}
	}

	return;
}

/* Does a quick meausre of how big the string is in
   pixels with a Pango layout */
static gint
measure_string (GtkStyle * style, PangoContext * context, const gchar * string)
{
	PangoLayout * layout = pango_layout_new(context);

	if (pango_parse_markup(string, -1, 0, NULL, NULL, NULL, NULL))
		pango_layout_set_markup(layout, string, -1);
	else
		pango_layout_set_text(layout, string, -1);

	pango_layout_set_font_description(layout, style->font_desc);

	gint width;
	pango_layout_get_pixel_size(layout, &width, NULL);
	g_object_unref(layout);
	return width;
}

/* Format for the table of strftime() modifiers to what
   we need to check when determining the length */
typedef struct _strftime_type_t strftime_type_t;
struct _strftime_type_t {
	char character;
	gint mask;
};

/* A table taken from the man page of strftime to what the different
   characters can effect.  These are worst case in that we need to
   test the length based on all these things to ensure that we have
   a reasonable string lenght measurement. */
const static strftime_type_t strftime_type[] = {
	{'a', STRFTIME_MASK_WEEK},
	{'A', STRFTIME_MASK_WEEK},
	{'b', STRFTIME_MASK_MONTH},
	{'B', STRFTIME_MASK_MONTH},
	{'c', STRFTIME_MASK_ALL}, /* We don't know, so we have to assume all */
	{'C', STRFTIME_MASK_YEAR},
	{'d', STRFTIME_MASK_MONTH},
	{'D', STRFTIME_MASK_MONTH | STRFTIME_MASK_YEAR | STRFTIME_MASK_DAY},
	{'e', STRFTIME_MASK_DAY},
	{'F', STRFTIME_MASK_MONTH | STRFTIME_MASK_YEAR | STRFTIME_MASK_DAY},
	{'G', STRFTIME_MASK_YEAR},
	{'g', STRFTIME_MASK_YEAR},
	{'h', STRFTIME_MASK_MONTH},
	{'j', STRFTIME_MASK_DAY},
	{'m', STRFTIME_MASK_MONTH},
	{'p', STRFTIME_MASK_AMPM},
	{'P', STRFTIME_MASK_AMPM},
	{'r', STRFTIME_MASK_AMPM},
	{'s', STRFTIME_MASK_SECONDS},
	{'S', STRFTIME_MASK_SECONDS},
	{'T', STRFTIME_MASK_SECONDS},
	{'u', STRFTIME_MASK_WEEK},
	{'U', STRFTIME_MASK_DAY | STRFTIME_MASK_MONTH},
	{'V', STRFTIME_MASK_DAY | STRFTIME_MASK_MONTH},
	{'w', STRFTIME_MASK_DAY},
	{'W', STRFTIME_MASK_DAY | STRFTIME_MASK_MONTH},
	{'x', STRFTIME_MASK_YEAR | STRFTIME_MASK_MONTH | STRFTIME_MASK_DAY | STRFTIME_MASK_WEEK},
	{'X', STRFTIME_MASK_SECONDS},
	{'y', STRFTIME_MASK_YEAR},
	{'Y', STRFTIME_MASK_YEAR},
	/* Last one */
	{0, 0}
};

#define FAT_NUMBER 8

/* Looks through the characters in the format string to
   ensure that we can figure out which of the things we
   need to check in determining the length. */
static gint
generate_strftime_bitmask (const char *time_str)
{
	gint retval = 0;
	glong strlength = g_utf8_strlen(time_str, -1);
	gint i;
	g_debug("Evaluating bitmask for '%s'", time_str);

	for (i = 0; i < strlength; i++) {
		if (time_str[i] == '%' && i + 1 < strlength) {
			gchar evalchar = time_str[i + 1];

			/* If we're using alternate formats we need to skip those characters */
			if (evalchar == 'E' || evalchar == 'O') {
				if (i + 2 < strlength) {
					evalchar = time_str[i + 2];
				} else {
					continue;
				}
			}

			/* Let's look at that character in the table */
			int j;
			for (j = 0; strftime_type[j].character != 0; j++) {
				if (strftime_type[j].character == evalchar) {
					retval |= strftime_type[j].mask;
					break;
				}
			}
		}
	}

	return retval;
}

/* Build an array up of all the time values that we want to check
   for length to ensure we're in a good place */
static void
build_timeval_array (GArray * timevals, gint mask)
{
	struct tm mytm = {0};

	/* Sun 12/28/8888 00:00 */
	mytm.tm_hour = 0;
	mytm.tm_mday = 28;
	mytm.tm_mon = 11;
	mytm.tm_year = 8888 - 1900;
	mytm.tm_wday = 0;
	mytm.tm_yday = 363;
	g_array_append_val(timevals, mytm);

	if (mask & STRFTIME_MASK_AMPM) {
		/* Sun 12/28/8888 12:00 */
		mytm.tm_hour = 12;
		g_array_append_val(timevals, mytm);
	}

	/* NOTE: Ignoring year 8888 should handle it */

	if (mask & STRFTIME_MASK_MONTH) {
		gint oldlen = timevals->len;
		gint i, j;
		for (i = 0; i < oldlen; i++) {
			for (j = 0; j < 11; j++) {
				struct tm localval = g_array_index(timevals, struct tm, i);
				localval.tm_mon = j;
				/* Not sure if I need to adjust yday & wday, hope not */
				g_array_append_val(timevals, localval);
			}
		}
	}

	/* Doing these together as it seems like just slightly more
	   coverage on the numerical days, but worth it. */
	if (mask & (STRFTIME_MASK_WEEK | STRFTIME_MASK_DAY)) {
		gint oldlen = timevals->len;
		gint i, j;
		for (i = 0; i < oldlen; i++) {
			for (j = 22; j < 28; j++) {
				struct tm localval = g_array_index(timevals, struct tm, i);

				gint diff = 28 - j;

				localval.tm_mday = j;
				localval.tm_wday = localval.tm_wday - diff;
				if (localval.tm_wday < 0) {
					localval.tm_wday += 7;
				}
				localval.tm_yday = localval.tm_yday - diff;

				g_array_append_val(timevals, localval);
			}
		}
	}

	return;
}

/* Try to get a good guess at what a maximum width of the entire
   string would be. */
static void
guess_label_size (IndicatorDatetime * self)
{
	/* This is during startup. */
	if (self->priv->label == NULL) return;

	GtkStyle * style = gtk_widget_get_style(GTK_WIDGET(self->priv->label));
	PangoContext * context = gtk_widget_get_pango_context(GTK_WIDGET(self->priv->label));
	gint * max_width = &(self->priv->max_width);
	gint posibilitymask = generate_strftime_bitmask(self->priv->time_string);

	/* Reset max width */
	*max_width = 0;

	/* Build the array of possibilities that we want to test */
	GArray * timevals = g_array_new(FALSE, TRUE, sizeof(struct tm));
	build_timeval_array(timevals, posibilitymask);

	g_debug("Checking against %d posible times", timevals->len);
	gint check_time;
	for (check_time = 0; check_time < timevals->len; check_time++) {
		gchar longstr[256];
		strftime(longstr, 256, self->priv->time_string, &(g_array_index(timevals, struct tm, check_time)));

		gchar * utf8 = g_locale_to_utf8(longstr, -1, NULL, NULL, NULL);
		gint length = measure_string(style, context, utf8);
		g_free(utf8);

		if (length > *max_width) {
			*max_width = length;
		}
	}

	g_array_free(timevals, TRUE);

	gtk_widget_set_size_request(GTK_WIDGET(self->priv->label), self->priv->max_width, -1);
	g_debug("Guessing max time width: %d", self->priv->max_width);

	return;
}

/* React to the style changing, which could mean an font
   update. */
static void
style_changed (GtkWidget * widget, GtkStyle * oldstyle, gpointer data)
{
	g_debug("New style for time label");
	IndicatorDatetime * self = INDICATOR_DATETIME(data);
	guess_label_size(self);
	update_label(self, NULL);
	timezone_update_all_labels(self);
	return;
}

/* Respond to changes in the screen to update the text gravity */
static void
update_text_gravity (GtkWidget *widget, GdkScreen *previous_screen, gpointer data)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(data);
	if (self->priv->label == NULL) return;

	PangoLayout  *layout;
	PangoContext *context;

	layout = gtk_label_get_layout (GTK_LABEL(self->priv->label));
	context = pango_layout_get_context(layout);
	pango_context_set_base_gravity(context, PANGO_GRAVITY_AUTO);
}

static gchar *
generate_format_string_now (IndicatorDatetime * self)
{
	if (self->priv->time_mode == SETTINGS_TIME_CUSTOM) {
		return g_strdup(self->priv->custom_string);
	}
	else {
		return generate_format_string_full(self->priv->show_day,
		                                   self->priv->show_date);
	}
}

static void
timezone_update_labels (indicator_item_t * mi_data)
{
	const gchar * zone = dbusmenu_menuitem_property_get(mi_data->mi, TIMEZONE_MENUITEM_PROP_ZONE);
	const gchar * name = dbusmenu_menuitem_property_get(mi_data->mi, TIMEZONE_MENUITEM_PROP_NAME);

	gtk_label_set_text(GTK_LABEL(mi_data->label), name);

	/* Show current time in that zone on the right */
	GTimeZone * tz = g_time_zone_new(zone);
	set_label_to_time_in_zone(mi_data->self, GTK_LABEL(mi_data->right), tz, NULL, NULL);
	g_time_zone_unref(tz);
}

/* Whenever we have a property change on a DbusmenuMenuitem
   we need to be responsive to that. */
static void
indicator_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GVariant *value, indicator_item_t * mi_data)
{
	if (!g_strcmp0(prop, APPOINTMENT_MENUITEM_PROP_LABEL)) {
		/* Set the main label */
		gtk_label_set_text(GTK_LABEL(mi_data->label), g_variant_get_string(value, NULL));
	} else if (!g_strcmp0(prop, APPOINTMENT_MENUITEM_PROP_RIGHT)) {
		/* Set the right label */
		gtk_label_set_text(GTK_LABEL(mi_data->right), g_variant_get_string(value, NULL));
	} else if (!g_strcmp0(prop, APPOINTMENT_MENUITEM_PROP_ICON)) {
		/* We don't use the value here, which is probably less efficient, 
		   but it's easier to use the easy function.  And since th value
		   is already cached, shouldn't be a big deal really.  */
		GdkPixbuf * pixbuf = dbusmenu_menuitem_property_get_image(mi, APPOINTMENT_MENUITEM_PROP_ICON);
		if (pixbuf != NULL) {
			/* If we've got a pixbuf we need to make sure it's of a reasonable
			   size to fit in the menu.  If not, rescale it. */
			GdkPixbuf * resized_pixbuf;
			gint width, height;
			gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);
			if (gdk_pixbuf_get_width(pixbuf) > width ||
					gdk_pixbuf_get_height(pixbuf) > height) {
				g_debug("Resizing icon from %dx%d to %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), width, height);
				resized_pixbuf = gdk_pixbuf_scale_simple(pixbuf,
				                                         width,
				                                         height,
				                                         GDK_INTERP_BILINEAR);
			} else {
				g_debug("Happy with icon sized %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
				resized_pixbuf = pixbuf;
			}
			gtk_image_set_from_pixbuf(GTK_IMAGE(mi_data->icon), resized_pixbuf);
			/* The other pixbuf should be free'd by the dbusmenu. */
			if (resized_pixbuf != pixbuf) {
				g_object_unref(resized_pixbuf);
			}
		}
	} else if (!g_strcmp0(prop, TIMEZONE_MENUITEM_PROP_ZONE)) {
		timezone_update_labels(mi_data);
	} else if (!g_strcmp0(prop, TIMEZONE_MENUITEM_PROP_NAME)) {
		timezone_update_labels(mi_data);
	} else if (!g_strcmp0(prop, TIMEZONE_MENUITEM_PROP_RADIO)) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi_data->gmi), g_variant_get_boolean(value));
	}
	return;
}
// Properties for marking and unmarking the calendar
static void
calendar_prop_change_cb (DbusmenuMenuitem * mi, gchar * prop, GVariant *value, IdoCalendarMenuItem * mi_data)
{
	g_debug("Changing calendar property: %s", prop);
	if (!g_strcmp0(prop, CALENDAR_MENUITEM_PROP_MARKS)) {
		ido_calendar_menu_item_clear_marks (IDO_CALENDAR_MENU_ITEM (mi_data));

		if (value != NULL) {
			GVariantIter *iter;
			gint day;

			g_debug("\tMarks: %s", g_variant_print(value, FALSE));

			g_variant_get (value, "ai", &iter);
			while (g_variant_iter_loop (iter, "i", &day)) {
				ido_calendar_menu_item_mark_day (IDO_CALENDAR_MENU_ITEM (mi_data), day);
			}
			g_variant_iter_free (iter);
		} else {
			g_debug("\tMarks: <cleared>");
		}
	}
	return;
}

/* We have a small little menuitem type that handles all
   of the fun stuff for indicators.  Mostly this is the
   shifting over and putting the icon in with some right
   side text that'll be determined by the service.  
   Copied verbatim from an old revision (including comments) of indicator-messages   
*/
static gboolean
new_appointment_item (DbusmenuMenuitem * newitem, DbusmenuMenuitem * parent, DbusmenuClient * client, gpointer user_data)
{
	g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
	g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
	g_return_val_if_fail(IS_INDICATOR_DATETIME(user_data), FALSE);
	/* Note: not checking parent, it's reasonable for it to be NULL */
	IndicatorDatetime * self = INDICATOR_DATETIME(user_data);

	indicator_item_t * mi_data = g_new0(indicator_item_t, 1);

	mi_data->gmi = gtk_menu_item_new();

	GtkWidget * hbox = gtk_hbox_new(FALSE, 4);

	/* Icon, probably someone's face or avatar on an IM */
	mi_data->icon = gtk_image_new();
	GdkPixbuf * pixbuf = dbusmenu_menuitem_property_get_image(newitem, APPOINTMENT_MENUITEM_PROP_ICON);

	if (pixbuf != NULL) {
		/* If we've got a pixbuf we need to make sure it's of a reasonable
		   size to fit in the menu.  If not, rescale it. */
		GdkPixbuf * resized_pixbuf;
		gint width, height;
		gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &width, &height);
		if (gdk_pixbuf_get_width(pixbuf) > width ||
		        gdk_pixbuf_get_height(pixbuf) > height) {
			g_debug("Resizing icon from %dx%d to %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf), width, height);
			resized_pixbuf = gdk_pixbuf_scale_simple(pixbuf,
			                                         width,
			                                         height,
			                                         GDK_INTERP_BILINEAR);
		} else {
			g_debug("Happy with icon sized %dx%d", gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
			resized_pixbuf = pixbuf;
		}
  
		gtk_image_set_from_pixbuf(GTK_IMAGE(mi_data->icon), resized_pixbuf);

		/* The other pixbuf should be free'd by the dbusmenu. */
		if (resized_pixbuf != pixbuf) {
			g_object_unref(resized_pixbuf);
		}
	}
	gtk_misc_set_alignment(GTK_MISC(mi_data->icon), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), mi_data->icon, FALSE, FALSE, 0);
	gtk_widget_show(mi_data->icon);

	/* Label, probably a username, chat room or mailbox name */
	mi_data->label = gtk_label_new(dbusmenu_menuitem_property_get(newitem, APPOINTMENT_MENUITEM_PROP_LABEL));
	gtk_misc_set_alignment(GTK_MISC(mi_data->label), 0.0, 0.5);
	
	GtkStyle * style = gtk_widget_get_style(GTK_WIDGET(mi_data->label));
	PangoContext * context = gtk_widget_get_pango_context(GTK_WIDGET(mi_data->label));
	gint length = measure_string(style, context, "MMMMMMMMMMMMMMM"); // 15 char wide string max
	gtk_widget_set_size_request(GTK_WIDGET(mi_data->label), length, -1); // Set the min size in pixels
	
	gtk_label_set_ellipsize(GTK_LABEL(mi_data->label), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start(GTK_BOX(hbox), mi_data->label, TRUE, TRUE, 0);
	gtk_widget_show(mi_data->label);

	/* Usually either the time or the count on the individual
	   item. */
	mi_data->right = gtk_label_new(dbusmenu_menuitem_property_get(newitem, APPOINTMENT_MENUITEM_PROP_RIGHT));
	gtk_size_group_add_widget(self->priv->indicator_right_group, mi_data->right);
	gtk_misc_set_alignment(GTK_MISC(mi_data->right), 1.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), mi_data->right, FALSE, FALSE, 0);
	gtk_widget_show(mi_data->right);

	gtk_container_add(GTK_CONTAINER(mi_data->gmi), hbox);
	gtk_widget_show(hbox);

	dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, GTK_MENU_ITEM(mi_data->gmi), parent);

	g_signal_connect(G_OBJECT(newitem), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(indicator_prop_change_cb), mi_data);
	return TRUE;
}

static void
month_changed_cb (IdoCalendarMenuItem *ido, 
                  gpointer        user_data) 
{
	guint d,m,y;
	DbusmenuMenuitem * item = DBUSMENU_MENUITEM (user_data);
	ido_calendar_menu_item_get_date(ido, &y, &m, &d);
	struct tm date = {0};
	date.tm_mday = d;
	date.tm_mon = m;
	date.tm_year = y - 1900;
	guint selecteddate = (guint)mktime(&date);
	g_debug("Got month changed signal: %s", asctime(&date));
	GVariant *variant = g_variant_new_uint32(selecteddate);
	guint timestamp = (guint)time(NULL);
	dbusmenu_menuitem_handle_event(DBUSMENU_MENUITEM(item), "month-changed", variant, timestamp);
}
	
static void
day_selected_cb (IdoCalendarMenuItem *ido,
                 gpointer        user_data) 
{
	guint d,m,y;
	DbusmenuMenuitem * item = DBUSMENU_MENUITEM (user_data);
	ido_calendar_menu_item_get_date(ido, &y, &m, &d);
	struct tm date = {0};
	date.tm_mday = d;
	date.tm_mon = m;
	date.tm_year = y - 1900;
	guint selecteddate = (guint)mktime(&date);
	g_debug("Got day selected signal: %s", asctime(&date));
	GVariant *variant = g_variant_new_uint32(selecteddate);
	guint timestamp = (guint)time(NULL);
	dbusmenu_menuitem_handle_event(DBUSMENU_MENUITEM(item), "day-selected", variant, timestamp);
}

static void
day_selected_double_click_cb (IdoCalendarMenuItem *ido,
                              gpointer        user_data) 
{
	guint d,m,y;
	DbusmenuMenuitem * item = DBUSMENU_MENUITEM (user_data);
	ido_calendar_menu_item_get_date(ido, &y, &m, &d);
	struct tm date = {0};
	date.tm_mday = d;
	date.tm_mon = m;
	date.tm_year = y - 1900;
	guint selecteddate = (guint)mktime(&date);
	g_debug("Got day selected double click signal: %s", asctime(&date));
	GVariant *variant = g_variant_new_uint32(selecteddate);
	guint timestamp = (guint)time(NULL);
	dbusmenu_menuitem_handle_event(DBUSMENU_MENUITEM(item), "day-selected-double-click", variant, timestamp);
}

static gboolean
new_calendar_item (DbusmenuMenuitem * newitem,
				   DbusmenuMenuitem * parent,
				   DbusmenuClient   * client,
				   gpointer           user_data)
{
	g_debug("New calendar item");
	g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
	g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
	g_return_val_if_fail(IS_INDICATOR_DATETIME(user_data), FALSE);
	/* Note: not checking parent, it's reasonable for it to be NULL */

	IndicatorDatetime *self = INDICATOR_DATETIME(user_data);
	self->priv = INDICATOR_DATETIME_GET_PRIVATE(self);
	
	IdoCalendarMenuItem *ido = IDO_CALENDAR_MENU_ITEM (ido_calendar_menu_item_new ());
	self->priv->ido_calendar = ido;
	
	GtkCalendarDisplayOptions flags = ido_calendar_menu_item_get_display_options (self->priv->ido_calendar);
	if (self->priv->show_week_numbers == TRUE)
		flags |= GTK_CALENDAR_SHOW_WEEK_NUMBERS;
	else
		flags &= ~GTK_CALENDAR_SHOW_WEEK_NUMBERS;
	ido_calendar_menu_item_set_display_options (self->priv->ido_calendar, flags);

	gtk_widget_set_visible (GTK_WIDGET (self->priv->ido_calendar), self->priv->show_calendar);

	dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, GTK_MENU_ITEM(ido), parent);

	g_signal_connect_after(ido, "month-changed", G_CALLBACK(month_changed_cb), (gpointer)newitem);
	g_signal_connect_after(ido, "day-selected", G_CALLBACK(day_selected_cb), (gpointer)newitem);
	g_signal_connect_after(ido, "day-selected-double-click", G_CALLBACK(day_selected_double_click_cb), (gpointer)newitem);

	g_signal_connect(G_OBJECT(newitem), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(calendar_prop_change_cb), ido);

	/* Run the current values through prop changed */
	GVariant * propval = NULL;

	propval = dbusmenu_menuitem_property_get_variant(newitem, CALENDAR_MENUITEM_PROP_MARKS);
	if (propval != NULL) {
		calendar_prop_change_cb(newitem, CALENDAR_MENUITEM_PROP_MARKS, propval, ido);
	}

	return TRUE;
}

static void
timezone_toggled_cb (GtkCheckMenuItem *checkmenuitem, DbusmenuMenuitem * dbusitem)
{
	/* Make sure that the displayed radio-active setting is always 
	   consistent with the dbus menuitem */
	gtk_check_menu_item_set_active(checkmenuitem,
		dbusmenu_menuitem_property_get_bool(dbusitem, TIMEZONE_MENUITEM_PROP_RADIO));
}

static void
timezone_destroyed_cb (indicator_item_t * mi_data, DbusmenuMenuitem * dbusitem)
{
	IndicatorDatetimePrivate *priv = INDICATOR_DATETIME_GET_PRIVATE(mi_data->self);
	priv->timezone_items = g_list_remove(priv->timezone_items, mi_data);
	g_signal_handlers_disconnect_by_func(G_OBJECT(mi_data->gmi), G_CALLBACK(timezone_toggled_cb), dbusitem);
	g_free(mi_data);
}

static gboolean
new_timezone_item(DbusmenuMenuitem * newitem,
				   DbusmenuMenuitem * parent,
				   DbusmenuClient   * client,
				   gpointer           user_data)
{
	g_return_val_if_fail(DBUSMENU_IS_MENUITEM(newitem), FALSE);
	g_return_val_if_fail(DBUSMENU_IS_GTKCLIENT(client), FALSE);
	g_return_val_if_fail(IS_INDICATOR_DATETIME(user_data), FALSE);
	/* Note: not checking parent, it's reasonable for it to be NULL */

	IndicatorDatetime * self = INDICATOR_DATETIME(user_data);
	IndicatorDatetimePrivate *priv = INDICATOR_DATETIME_GET_PRIVATE(self);

	// Menu item with a radio button and a right aligned time
	indicator_item_t * mi_data = g_new0(indicator_item_t, 1);

	priv->timezone_items = g_list_prepend(priv->timezone_items, mi_data);

	mi_data->self = self;
	mi_data->mi = newitem;
	mi_data->gmi = gtk_check_menu_item_new();

	gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(mi_data->gmi), TRUE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi_data->gmi),
		dbusmenu_menuitem_property_get_bool(newitem, TIMEZONE_MENUITEM_PROP_RADIO));

	GtkWidget * hbox = gtk_hbox_new(FALSE, 4);

  	/* Label, probably a username, chat room or mailbox name */
	mi_data->label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(mi_data->label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), mi_data->label, TRUE, TRUE, 0);
	gtk_widget_show(mi_data->label);

	/* Usually either the time or the count on the individual
	   item. */
	mi_data->right = gtk_label_new("");
	gtk_size_group_add_widget(self->priv->indicator_right_group, mi_data->right);
	gtk_misc_set_alignment(GTK_MISC(mi_data->right), 1.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), mi_data->right, FALSE, FALSE, 0);
	gtk_widget_show(mi_data->right);

	timezone_update_labels(mi_data);

	gtk_container_add(GTK_CONTAINER(mi_data->gmi), hbox);
	gtk_widget_show(hbox);

	dbusmenu_gtkclient_newitem_base(DBUSMENU_GTKCLIENT(client), newitem, GTK_MENU_ITEM(mi_data->gmi), parent);

	g_signal_connect(G_OBJECT(mi_data->gmi), "toggled", G_CALLBACK(timezone_toggled_cb), newitem);
	g_signal_connect(G_OBJECT(newitem), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(indicator_prop_change_cb), mi_data);
	g_object_weak_ref(G_OBJECT(newitem), (GWeakNotify)timezone_destroyed_cb, mi_data);

	return TRUE;
}

/* Grabs the label.  Creates it if it doesn't
   exist already */
static GtkLabel *
get_label (IndicatorObject * io)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(io);

	/* If there's not a label, we'll build ourselves one */
	if (self->priv->label == NULL) {
		self->priv->label = GTK_LABEL(gtk_label_new("Time"));
		gtk_label_set_justify (GTK_LABEL(self->priv->label), GTK_JUSTIFY_CENTER);
		g_object_ref(G_OBJECT(self->priv->label));
		g_signal_connect(G_OBJECT(self->priv->label), "style-set", G_CALLBACK(style_changed), self);
		g_signal_connect(G_OBJECT(self->priv->label), "screen-changed", G_CALLBACK(update_text_gravity), self);
		guess_label_size(self);
		update_label(self, NULL);
		gtk_widget_set_visible(GTK_WIDGET (self->priv->label), self->priv->show_clock);
	}

	if (self->priv->timer == 0) {
		setup_timer(self, NULL);
	}

	return self->priv->label;
}

static GtkMenu *
get_menu (IndicatorObject * io)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(io);

	return GTK_MENU(self->priv->menu);
}

static const gchar *
get_accessible_desc (IndicatorObject * io)
{
	IndicatorDatetime * self = INDICATOR_DATETIME(io);
	const gchar * name;

	if (self->priv->label != NULL) {
		name = gtk_label_get_text(self->priv->label);
		return name;
	}
	return NULL;
}
