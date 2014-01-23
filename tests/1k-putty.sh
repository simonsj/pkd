#!/bin/bash
#
# 1k-putty.sh [path-to-key]
#
#   Issue a dummy "ls" exec command one thousand
#   times and ensure that each one succeeds.
#

if [ $# -eq 0 ]; then
  echo "usage: 1k-putty.sh [path-to-id-key]"
  exit 1
fi

ID="-i $1"
HOST="localhost"
PORT="1234"
OPTS="-noagent"

OUT="/tmp/1k-openssh-out"

for n in $(seq 1000); do
  putty/plink $ID $OPTS $HOST -P $PORT "ls" &> $OUT
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
