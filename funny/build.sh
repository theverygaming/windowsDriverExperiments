#!/usr/bin/env bash
set -e

make clean
make -j12
cp drv.sys /srv/http

cd stage2
make -j12
cd ..

cp stage2/stage2.bin /srv/http

echo "OK!!"
