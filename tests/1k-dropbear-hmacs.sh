#!/bin/bash
#
# 1k-dropbear-hmacs.sh [path-to-key]
#
#   Issue a dummy "ls" exec command one thousand
#   times and ensure that each one succeeds.
#   Cycle through different HMAC algorithms.
#

if [ $# -eq 0 ]; then
  echo "usage: 1k-dropbear-hmacs.sh [path-to-id-key]"
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

HMACS="
hmac-sha1
hmac-sha2-256
hmac-sha2-512
"

for nnn in $(seq 10); do
  for h in $HMACS; do
    for n in $(seq 1000); do
      $SSH -vvv $ID $OPTS -m $h $HOST -p $PORT "ls" &> $OUT
      if [ $? -ne 0 ]; then
        cat  $OUT
        mv $OUT tests-out
        echo "($nnn) FAILED (run $n) ($h)" && exit 1
      fi
      rm $OUT
      echo "($nnn) OK (run $n) ($h)"
    done

    echo "($nnn) PASSED ($n runs) ($h)"
  done
done

exit 0
