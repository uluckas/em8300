#!/bin/bash

kvers=`uname -r | awk -F '.' '{ print $2 }'`

cd modules
if [ $kvers -eq 4 ]; then
  echo 'A 2.4 kernel-structure'
  cp Makefile.2.4 Makefile
else
  echo 'A 2.2 kernel-structure'
  cp Makefile.2.2 Makefile
fi
cd ..
