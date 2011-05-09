#!/bin/bash

PREFIX_NAME="$HOME/Projects/indicators/example/build"

./autogen.sh --prefix="$PREFIX_NAME" --libdir="$PREFIX_NAME" --enable-localinstall
