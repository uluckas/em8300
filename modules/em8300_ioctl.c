#define __NO_VERSION__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/malloc.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/i2c-algo-bit.h>

#include "em8300_reg.h"

#include "em8300.h"

#include "encoder.h"

int em8300_ioctl_setvideomode(struct em8300_s *em, int mode) {
    int encoder;

    switch(mode) {
    case EM8300_VIDEOMODE_PAL:
	encoder = ENCODER_MODE_PAL;
	break;
    case EM8300_VIDEOMODE_PAL60:
	encoder = ENCODER_MODE_PAL60;
	break;
    case EM8300_VIDEOMODE_NTSC:
	encoder = ENCODER_MODE_NTSC;
	break;
    default:
	return -EINVAL;
    }

    em->video_mode = mode;
    
    em8300_dicom_update(em);

    if(em->encoder)
	em->encoder->driver->command(em->encoder,ENCODER_CMD_SETMODE,
				     (void *)encoder);
    return 0;
}

void em8300_ioctl_setBCS(struct em8300_s *em, int brightness, int chroma, int saturation) {
    em->dicom_brightness = 0x40;
    em->dicom_chroma = 0x60;
    em->dicom_saturation = 0x60;

    em8300_dicom_update(em);
}

int em8300_ioctl_setaspectratio(struct em8300_s *em, int ratio) {

    em->aspect_ratio = ratio;
    
    em8300_dicom_update(em);

    return 0;
}

int em8300_ioctl_setplaymode(struct em8300_s *em, int mode) {
    switch(mode) {
    case EM8300_PLAYMODE_PLAY:
	
    }
    return 0;
}

