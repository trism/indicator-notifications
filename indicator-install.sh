#!/bin/bash

PREFIX_NAME="$HOME/Projects/indicators/example/build"

cp -v "$PREFIX_NAME/share/dbus-1/services/indicator-example.service" "/usr/share/dbus-1/services/"
cp -v "$PREFIX_NAME/indicators/2/libexample.so" "/usr/lib/indicators/5/"
