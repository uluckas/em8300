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

struct dicom_tvmode {
    int vertsize;
    int horizsize;
    int vertoffset;
    int horizoffset;
};

struct dicom_tvmode tvmodematrix[EM8300_VIDEOMODE_LAST+1][2] = {
    { {576, 720, 46, 130},	// PAL 4:3 
      {480, 720, 40, 130}},	// PAL 16:9
    { {480, 720, 46, 138},	// PAL60 4:3 
      {480, 720, 46, 138}},	// PAL60 16:9
    { {480, 720, 46, 138},	// NTSC 4:3 
      {480, 720, 46, 138}},	// NTSC 16:9
};

int em8300_dicom_update(struct em8300_s *em) {
    int ret;
    
    if( (ret=em8300_waitfor(em, ucregister(DICOM_UpdateFlag), 0, 1)) )
	return ret;

    write_ucregister(DICOM_BCSLuma, em->dicom_brightness << 8);
    write_ucregister(DICOM_BCSChroma, em->dicom_saturation | (em->dicom_chroma << 8));
    write_ucregister(DICOM_FrameTop, tvmodematrix[em->video_mode][em->aspect_ratio].vertoffset);
    write_ucregister(DICOM_FrameBottom, tvmodematrix[em->video_mode][em->aspect_ratio].vertoffset +
		                        tvmodematrix[em->video_mode][em->aspect_ratio].vertsize-1);
    write_ucregister(DICOM_FrameLeft, tvmodematrix[em->video_mode][em->aspect_ratio].horizoffset);
    write_ucregister(DICOM_FrameRight, tvmodematrix[em->video_mode][em->aspect_ratio].horizoffset +
		                       tvmodematrix[em->video_mode][em->aspect_ratio].horizsize-1);
    write_ucregister(DICOM_VisibleTop, tvmodematrix[em->video_mode][em->aspect_ratio].vertoffset);
    write_ucregister(DICOM_VisibleBottom, tvmodematrix[em->video_mode][em->aspect_ratio].vertoffset +
		                        tvmodematrix[em->video_mode][em->aspect_ratio].vertsize-1);
    write_ucregister(DICOM_VisibleLeft, tvmodematrix[em->video_mode][em->aspect_ratio].horizoffset);
    write_ucregister(DICOM_VisibleRight, tvmodematrix[em->video_mode][em->aspect_ratio].horizoffset +
		                       tvmodematrix[em->video_mode][em->aspect_ratio].horizsize-1);

    write_ucregister(DICOM_TvOut, 0x4001);

    if((em->encoder_type == ENCODER_ADV7170) ||
       ((em->encoder_type == ENCODER_ADV7175) &&
       (em->video_mode == EM8300_VIDEOMODE_NTSC)))
        write_ucregister(DICOM_Control,0x9efe);
    
    write_ucregister(DICOM_UpdateFlag,1);
    
    return em8300_waitfor(em, ucregister(DICOM_UpdateFlag), 0, 1);
}

