#!/bin/bash
#
# 1k-openssh.sh [path-to-key]
#
#   Issue a dummy "ls" exec command one thousand
#   times and ensure that each one succeeds.
#

if [ $# -eq 0 ]; then
  echo "usage: 1k-openssh.sh [path-to-id-key]"
  exit 1
fi

SSH="ssh"
ID="-i $1"
HOST="localhost"
PORT="1234"
OPTS="-vvv -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null"

OUT="/tmp/1k-openssh-out"

for n in $(seq 1000); do
  $SSH -vvv $ID $OPTS $HOST -p $PORT "ls" &> $OUT
  if [ $? -ne 0 ]; then
    cat  $OUT
    mv $OUT tests-out
    echo "FAILED (run $n)" && exit 1
  fi
  rm $OUT
  echo "OK (run $n)"
done

echo "PASSED ($n runs)"
exit 0
