#!/bin/bash

rm -v "/usr/share/dbus-1/services/indicator-notifications.service"
rm -v "$(pkg-config --variable=indicatordir indicator3-0.4)/libnotifications.so"
