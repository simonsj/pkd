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

SSH="dl/putty/plink"
ID="-i $1"
HOST="localhost"
PORT="1234"
OPTS="-noagent -v"

OUT="/tmp/1k-putty-out"

#
# putty doesn't provide the equivalent of OpenSSH's -o StrictHostKeyChecking=no.
#
rm ~/.putty/sshhostkeys
yes | $SSH $ID $OPTS $HOST -P $PORT "accepting host key"

for n in $(seq 1000); do
  $SSH $ID $OPTS $HOST -P $PORT "ls" &> $OUT
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
