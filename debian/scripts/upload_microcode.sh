#!/bin/sh
#
# Upload microcode
#
# Usage: upload_mc [microcode]
#
if [ "$#" -lt 1 ]; then
   /usr/share/em8300/microcode_upload.pl /etc/dxr3.ux
else
   /usr/share/em8300/microcode_upload.pl "$1"
fi
