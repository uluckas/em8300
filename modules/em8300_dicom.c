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


// C decompilation of the chromaluma code
// by Anton Altaparmakov <antona@bigfoot.com>

// This basically returns the result of calculating param1 / param2 and
// depending on some weird rules either adds 1 to the result or not.
// Returns 0 on error, ie. when param2 is 0.
int sub_265C1 (int param1, unsigned int param2, short int param3)
{
	int local1;
	if (!param2)
		return 0;
	local1 = param1 / param2;
	if (param2 & 1) {
		param1 <<= 1;
		param2 <<= 1;
	}
	if (param3) {
		// Original was stupid, so there:
		if (param1 % param2 >= param2)
			++local1;
	} else
                if (param1 % param2 >= param2 >> 1)
	                ++local1;
	return local1;
}

/* sub_40137 calculates the contents of the dicom_bcsluma and dicom_bcschroma
*	     registers with brightness contrast and saturation values as inputs.
*	     I guess these are stored at [dword_250c4]+84h,88h,8ch.
*	     Correct! But only experimenting will tell which is 
*	     which and what scale they work on, and what the 
*	     minimum and maximum values are...
*/
int sub_40137(struct em8300_s *em)
{
	short int local1;
	int local2, local3;
	short int local4;
	int local5;
	short int local6, local7;

	local3 = sub_265C1(em->dicom_contrast * 0x7f, 0x3e8, 0); // 0x3e8 = 1000
	local2 = sub_265C1(em->dicom_brightness * 0xff, 0x3e8, 0);
	local5 = sub_265C1(em->dicom_saturation * 0x1f, 0x3e8, 0);
	if (local3 >= 0x40) {
		local1 = 0x2000 / (0xc0 - local3);
		{	int ir;
			ir = local2 - 0x80 - ((local3 - 0x40) << 7) / (0xc0 - local3);
			if (ir > -128)
				local4 = ir;
			else
				local4 = -128;
		}
		{	register int eax;
			if ((eax = local5 << 7) < 0)
				eax += 0xf;
			// The original did >> 4 and then << 6 but IMHO it's
			// probably faster this way, I might be wrong of course:
			local6 = local7 = ((eax & 0xfffffff0) << 2) / (0xc0 - local3);
		}
	} else {
		register int eax;
		if ((eax = (local3 + 0x40) << 6) < 0)
			eax += 0x7f;
		local1 = eax >> 7;
		if ((eax = local2 - local3 - 0x40) < 0x7f)
			local4 = eax;
		else
			local4 = 0x7f;
		if ((eax = local5 * (local3 + 0x40)) < 0)
			eax += 0xf;
		// The original did >> 4 and then << 6 but IMHO it's
		// probably faster this way, I might be wrong of course:
		if ((eax = (eax & 0xfffffff0) << 2) < 0)
			eax += 0x7f;
		local6 = local7 =  eax >> 7;
	}

	// Henrik: This is already done in em8300_dicom_update
	//	if (sub_readregister(DICOM_UpdateFlags + 0x1000) == 1) {
	//	    sub_writeregister(DICOM_UpdateFlags + 0x1000, 0);
	//	    udelay(1);
	//	}
	
	// FIXME: Might need to cast the localX to int to get sign extension.
	// NOTE: Is automatic if the second parameter to sub_writeregister is
	// defined as int. -> Check definition of sub_writeregister.
	write_ucregister(DICOM_BCSLuma, (local1 << 8) | (local4 & 0xff));
	write_ucregister(DICOM_BCSChroma, local7 << 8 | local6);
	// Henrik: It's better to update the internal dicom registers in em8300_dicom_update
	//	write_ucregister(DICOM_UpdateFlags + 0x1000, 1);
	
	
	return 1;
}

void em8300_dicom_setBCS(struct em8300_s *em, int brightness, int contrast, int saturation) {
    em->dicom_brightness = brightness;
    em->dicom_contrast = contrast;
    em->dicom_saturation = saturation;

    em8300_dicom_update(em);
}

int em8300_dicom_update(struct em8300_s *em) {
    int ret;
    
    if( (ret=em8300_waitfor(em, ucregister(DICOM_UpdateFlag), 0, 1)) )
	return ret;

    sub_40137(em); // Update brightness/contrast/saturation
    
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

