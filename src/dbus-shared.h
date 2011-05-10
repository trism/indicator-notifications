/*
An indicator to display recent notifications.

Adapted from: indicator-datetime/src/dbus-shared.c by
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

#define SERVICE_NAME    "com.launchpad.RecentNotifications.indicator"
#define SERVICE_IFACE   "com.launchpad.RecentNotifications.indicator.service"
#define SERVICE_OBJ     "/com/launchpad/RecentNotifications/indicator/service"
#define SERVICE_VERSION 1

#define MENU_OBJ        "/com/launchpad/RecentNotifications/indicator/menu"
