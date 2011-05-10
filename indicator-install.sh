#!/bin/bash

PREFIX_NAME="$HOME/Projects/indicators/notifications/build"

cp -v "$PREFIX_NAME/share/dbus-1/services/indicator-notifications.service" "/usr/share/dbus-1/services/"
cp -v "$PREFIX_NAME/indicators/2/libnotifications.so" "/usr/lib/indicators/5/"
