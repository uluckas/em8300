#!/bin/sh
#
# Upload microcode
#
# Usage: upload_mc [microcode]
#
if [ "$#" -lt 1 ]; then
   [ -x /usr/share/em8300/microcode_upload.pl ] && /usr/share/em8300/microcode_upload.pl /etc/dxr3.ux
else
   [ -x /usr/share/em8300/microcode_upload.pl ] && /usr/share/em8300/microcode_upload.pl "$1"
fi
