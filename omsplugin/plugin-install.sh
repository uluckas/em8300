#!/bin/sh
cp -a dxr3 $1/src/plugin/decode
cp Makefile.am $1/src/plugin/decode
cp Makefile.am.plugin $1/src/plugin/Makefile.am
cp bootstrap $1
echo "Now you are ready to build oms."