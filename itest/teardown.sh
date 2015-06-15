#!/bin/bash

[ -n "$ITEST_PREFIX" ] || ITEST_PREFIX=/tmp/itest

kill_service() {
if [ -s $1.pid ]; then
echo "Shutting down $1"
kill `cat $1.pid`
fi
}

if [ -d ${ITEST_PREFIX} ]; then
cd ${ITEST_PREFIX}
kill_service nginx
kill_service ennodb
cd ..
rm -rf ${ITEST_PREFIX}
fi
