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


#define  SERVICE_NAME     "com.canonical.indicator.datetime"
#define  SERVICE_IFACE    "com.canonical.indicator.datetime.service"
#define  SERVICE_OBJ      "/com/canonical/indicator/datetime/service"
#define  SERVICE_VERSION  1

#define  MENU_OBJ      "/com/canonical/indicator/datetime/menu"

#define DBUSMENU_CALENDAR_MENUITEM_TYPE    "x-canonical-calendar-item"

#define CALENDAR_MENUITEM_PROP_MARKS       "calendar-marks"

#define APPOINTMENT_MENUITEM_TYPE          "appointment-item"	
#define APPOINTMENT_MENUITEM_PROP_LABEL    "appointment-label"
#define APPOINTMENT_MENUITEM_PROP_ICON     "appointment-icon"
#define APPOINTMENT_MENUITEM_PROP_RIGHT    "appointment-time"

#define TIMEZONE_MENUITEM_TYPE             "timezone-item"	
#define TIMEZONE_MENUITEM_PROP_ZONE        "timezone-zone"	
#define TIMEZONE_MENUITEM_PROP_NAME        "timezone-name"
#define TIMEZONE_MENUITEM_PROP_RADIO       "timezone-radio"
