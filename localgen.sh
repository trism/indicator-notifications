#!/bin/bash

PREFIX_NAME="$HOME/Projects/indicator-example/build"

./autogen.sh --prefix="$PREFIX_NAME" --libdir="$PREFIX_NAME" --enable-localinstall
