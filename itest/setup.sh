#!/bin/bash
set -e

ROOT=`pwd`
[ -n "$ITEST_PREFIX" ] || ITEST_PREFIX=/tmp/itest

make ennodb
mkdir -p ${ITEST_PREFIX}/nginx
[ -e ${ITEST_PREFIX}/ennodb.socket ] || spawn-fcgi -P ${ITEST_PREFIX}/ennodb.pid -s ${ITEST_PREFIX}/ennodb.socket -- ./ennodb itest/ennodb.ini
/usr/sbin/nginx -c $ROOT/itest/nginx.conf -p ${ITEST_PREFIX} -g "pid ${ITEST_PREFIX}/nginx.pid;"

if [ "$1" = "-e" ]; then
export ITEST_HOST=localhost
export ITEST_PORT=8080
fi
