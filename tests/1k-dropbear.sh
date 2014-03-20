#!/bin/bash
#
# 1k-dropbear.sh [path-to-key]
#
#   Issue a dummy "ls" exec command one thousand
#   times and ensure that each one succeeds.
#

if [ $# -eq 0 ]; then
  echo "usage: 1k-dropbear.sh [path-to-id-key]"
  exit 1
fi

SSH="dl/dropbear/dbclient"
ID="-i $1"
HOST="localhost"
PORT="1234"
OPTS="-y -y"

OUT="/tmp/1k-dropbear-out"

unset SSH_AUTH_SOCK
chmod go-rw ./keys/*

for n in $(seq 1000); do
  $SSH $ID $OPTS $HOST -p $PORT "ls" &> $OUT
  if [ $? -ne 0 ]; then
    cat $OUT
    mv $OUT tests-out
    echo "FAILED (run $n)" && exit 1
  fi
  rm $OUT
  echo "OK (run $n)"
done

echo "PASSED ($n runs)"
exit 0
