#! /bin/sh
# This is a simple scripts to create the devices for em8300 driver,
# it should abort if you have the devfs filesystem mounted, em8300
# supports devfs so there should be no need.

DEVFS=`cat /proc/mounts | grep " devfs "`

if [ -z "$DEVFS" ] ; then
	echo "Devfs not mounted creating device nodes"
	mknod /dev/em8300-0    c 121 0
	ln -fs /dev/em8300-0 /dev/em8300
	mknod /dev/em8300_mv-0 c 121 1
	ln -fs /dev/em8300_mv-0 /dev/em8300_mv
	mknod /dev/em8300_ma-0 c 121 2
	ln -fs /dev/em8300_ma-0 /dev/em8300_ma
	mknod /dev/em8300_sp-0 c 121 3
	ln -fs /dev/em8300_sp-0 /dev/em8300_sp
	chmod 666 /dev/em8300*
else
	echo "Hmmm, looks like you have devfs mounted, Im going to be safe and leave things alone!"
fi

