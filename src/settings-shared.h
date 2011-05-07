/*
An indicator to show date and time information.

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

#ifndef __DATETIME_SETTINGS_SHARED_H__
#define __DATETIME_SETTINGS_SHARED_H__

#define SETTINGS_INTERFACE              "com.canonical.indicator.datetime"
#define SETTINGS_SHOW_CLOCK_S           "show-clock"
#define SETTINGS_TIME_FORMAT_S          "time-format"
#define SETTINGS_SHOW_SECONDS_S         "show-seconds"
#define SETTINGS_SHOW_DAY_S             "show-day"
#define SETTINGS_SHOW_DATE_S            "show-date"
#define SETTINGS_CUSTOM_TIME_FORMAT_S   "custom-time-format"
#define SETTINGS_SHOW_CALENDAR_S        "show-calendar"
#define SETTINGS_SHOW_WEEK_NUMBERS_S    "show-week-numbers"
#define SETTINGS_SHOW_EVENTS_S          "show-events"
#define SETTINGS_SHOW_LOCATIONS_S       "show-locations"
#define SETTINGS_LOCATIONS_S            "locations"
#define SETTINGS_TIMEZONE_NAME_S        "timezone-name"

enum {
	SETTINGS_TIME_LOCALE = 0,
	SETTINGS_TIME_12_HOUR = 1,
	SETTINGS_TIME_24_HOUR = 2,
	SETTINGS_TIME_CUSTOM = 3
};

/* TRANSLATORS: A format string for the strftime function for
   a clock showing 12-hour time without seconds. */
#define DEFAULT_TIME_12_FORMAT   N_("%l:%M %p")

/* TRANSLATORS: A format string for the strftime function for
   a clock showing 24-hour time without seconds. */
#define DEFAULT_TIME_24_FORMAT   N_("%H:%M")

#define DEFAULT_TIME_FORMAT          DEFAULT_TIME_12_FORMAT
#define DEFAULT_TIME_FORMAT_WITH_DAY DEFAULT_TIME_12_FORMAT_WITH_DAY

/* TRANSLATORS: A format string for the strftime function for
   a clock showing the day of the week and the time in 12-hour format without
   seconds. */
#define DEFAULT_TIME_12_FORMAT_WITH_DAY N_("%a %l:%M %p")

/* TRANSLATORS: A format string for the strftime function for
   a clock showing the day of the week and the time in 24-hour format without
   seconds.  Information is available in this Launchpad answer:
   https://answers.launchpad.net/ubuntu/+source/indicator-datetime/+question/149752 */
#define DEFAULT_TIME_24_FORMAT_WITH_DAY N_("%a %H:%M")

#endif
