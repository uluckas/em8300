#!/bin/sh

echo "$KVERS" > debian/KVERS

rm -f debian/control.tmp
rm -f debian/control

cat debian/source.control > debian/control.tmp
sed -e 's/\${kvers}/'"$KVERS"'/g' debian/em8300-modules.control | \
  tee -a debian/control >> debian/control.tmp

dpkg-gencontrol -p"em8300-modules-$KVERS" -Pdebian/tmp-modules \
  -cdebian/control.tmp

exit 0
