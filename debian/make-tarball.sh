#!/bin/sh

PWD=`pwd`

if [ ! -f debian/em8300.tar.gz ]; then
    cd /usr/src
    tar cvzf em8300.tar.gz modules/em8300/
    mv em8300.tar.gz modules/em8300/debian/em8300.tar.gz
    cd $PWD
fi
