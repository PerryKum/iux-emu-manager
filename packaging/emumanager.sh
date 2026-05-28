#!/bin/sh
PROGDIR="/storage/iux/emumanager"

cd "$PROGDIR" || exit 1

export LD_LIBRARY_PATH="/usr/lib/aarch64-linux-gnu:/usr/lib"
unset SDL_VIDEODRIVER

exec "$PROGDIR/emumanager"
