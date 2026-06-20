#!/bin/bash
# Skrypt budujący GTAsaDC

# Załaduj środowisko KOS
. /home/krusz/dc/kos/environ.sh

# Zbuduj projekt
make "$@"
