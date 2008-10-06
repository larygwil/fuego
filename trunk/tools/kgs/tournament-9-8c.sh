#!/bin/bash
# Script for playing Fuego9 in a tournament on 9x9 KGS on a machine with
# 8 cores / 8 GB

FUEGO="../../fuegomain/fuego"
NAME=Fuego9

usage() {
  cat >&2 <<EOF
Usage: $0 [options]
Options:
  -h Print help and exit
EOF
}

MAXGAMES_OPTION=""
while getopts "h" O; do
case "$O" in
  h)   usage; exit 0;;
  [?]) usage; exit 1;;
esac
done

shift $(($OPTIND - 1))
if [ $# -gt 0 ]; then
  usage
  exit 1
fi


echo "Enter KGS password for $NAME:"
read PASSWORD

GAMES_DIR="games/tournament/$NAME"
mkdir -p "$GAMES_DIR"

cat <<EOF >config-tournament-9-8c.gtp
# This file is auto-generated by tournament-9-8c.sh. Do not edit.

go_param debug_to_comment 1
go_param auto_save $GAMES_DIR/$NAME-

# UCT player parameters
# A node size is currently 56 bytes on a 64-bit machine, so a main memory
# of 7.3 GB can contain two trees (of the search and the init tree used for
# reuse_subtree) of about 65.000.000 nodes each
uct_param_player max_nodes 65000000
uct_param_player max_games 999999999
uct_param_player ignore_clock 0
uct_param_player reuse_subtree 1
uct_param_player ponder 1

# Set KGS rules (Chinese, positional superko)
go_rules kgs

book_load ../../book/book.dat

sg_param time_mode real
uct_param_search number_threads 8
uct_param_search lock_free 1
EOF

cat >tmp.cfg <<EOF
name=$NAME
password=$PASSWORD
room=Computer Go
mode=tournament
verbose=t
engine=$FUEGO -size 9 -config config-tournament-9-8c.gtp
reconnect=t
EOF
java -jar kgsGtp.jar tmp.cfg && rm -f tmp.cfg

#-----------------------------------------------------------------------------
