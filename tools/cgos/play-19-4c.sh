#!/bin/bash

# Script for playing Fuego on 19x19 CGOS on a machine with 4 cores / 4 GB

FUEGO="../../build/opt/fuegomain/fuego"
VERSION=$(cd ../..; svnversion) || exit 1
DEFAULT_NAME=Fuego-$VERSION-4c

echo "Enter CGOS name (default=$DEFAULT_NAME):"
read NAME
if [[ "$NAME" == "" ]]; then
    NAME="$DEFAULT_NAME"
fi
echo "Enter CGOS password for $NAME:"
read PASSWORD

GAMES_DIR="games-19/$NAME"
mkdir -p "$GAMES_DIR"

cat <<EOF >config-19-4c.gtp
# This file is auto-generated by play-19-4c.sh. Do not edit.

# Best performance settings for CGOS
# Uses the time limits, therefore the performance depends on the hardware.

go_param debug_to_comment 1
go_param auto_save $GAMES_DIR/$NAME-

uct_param_player reuse_subtree 1
uct_param_player ponder 1
uct_param_player early_pass 0

# Set CGOS rules (Tromp-Taylor, positional superko)
go_rules cgos

sg_param time_mode real
uct_param_search number_threads 4
uct_param_search lock_free 1
EOF

# Append 2>/dev/stderr to invocation, otherwise cgos3.tcl will not pass
# through stderr of the Go program
./cgos3-19.patched.tcl "$NAME" "$PASSWORD" \
  "$FUEGO --config config-19-4c.gtp 2>/dev/stderr" \
  gracefully_exit_server-19-4c
