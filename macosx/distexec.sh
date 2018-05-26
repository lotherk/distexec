#!/bin/bash
DEX_PATH="$(cd "$( dirname $0)" && pwd)"
export DYLD_LIBRARY_PATH="$DEX_PATH/lib/:$DYLD_LIBRARY_PATH"
$DEX_PATH/bin/distexec --plugin-path $DEX_PATH/share/distexec/plugins $@
