#! /bin/sh
# This is a simple scripts to create the devices for em8300 driver,
# it should abort if you have the devfs filesystem mounted, em8300
# supports devfs so there should be no need.

DEVFS=`cat /proc/mounts | grep devfs`

if [ -z "$DEVFS" ] ; then
	echo "Devfs not mounted creating device nodes"
	mknod /dev/em8300    c 121 0
	mknod /dev/em8300_mv c 121 1
	mknod /dev/em8300_ma c 121 2
	mknod /dev/em8300_sp c 121 3
	chmod 666 /dev/em8300*
else
	echo "Hmmm, looks like you have devfs mounted, Im going to be safe and leave things alone!"
fi

