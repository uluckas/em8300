#include <unistd.h>
#include <stdio.h>

#include "dxr3-api.h"

static int dxr3FD = -1;

int dxr3_init(int _dxr3FD)
{ 
  dxr3FD = _dxr3FD;

  return dxr3_reset();
}


int dxr3_reset()
{
  return  dxr3FD<0 ? -1 : 1;
  //dxr3 needs no reset
}

int dxr3_install_firmware(int dxr3FD, em8300_microcode_t *uCode)
{ 
  if(uCode != NULL) 
      return dxr3FD<0 ? -1 : ioctl(dxr3FD, EM8300_IOCTL_INIT, uCode);
  else
    return -1;
}
