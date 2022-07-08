#!/bin/sh
rm data/*
rm check.md5
# make dir for recevie data
if [ ! -d dataReceive ]; then
    mkdir dataReceive
fi

for i in `seq 0 999`
do
    cat /dev/urandom | head -c 102400 > data/data$i
done
cd data
md5sum $(find . -type f) | tee ../check.md5
cd ..

# not loop back address case
# HOST=127.0.0.1
# USER=pi
# DIR=demo/
# scp check.md5 ${USER}@${HOST}:${DIR}
