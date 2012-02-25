#!/bin/bash

PREFIX_NAME="$HOME/Projects/indicators/notifications/build"

cp -v "$PREFIX_NAME/indicators/2/libnotifications.so" "$(pkg-config --variable=indicatordir indicator3-0.4)"
