#!/bin/sh

#KVERS=`uname -r`

#echo "$KVERS" > debian/KVERS

rm -f debian/control.tmp

sed -e 's/\${kvers}/'"$KVERS"'/g' debian/em8300-modules.control > \
  debian/control.tmp

dpkg-gencontrol -p"$PKGMOD" -Pdebian/$PKGMOD \
  -cdebian/control.tmp

exit 0
