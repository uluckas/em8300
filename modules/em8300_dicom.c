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

static struct dicom_s dicom_PAL   =
{
   /* Luma */            0x4000,
    /* Chroma */          0x6060,
    /* Frame top */       0x2e, 
    /* Frame bottom */    0x26d,
    /* Frame left   */    0x8a,
    /* Frame right  */    0x359,
    /* Visible top */     0x2e, 
    /* Visible bottom */  0x26d,
    /* Visible left   */  0x8a,
    /* Visible right  */  0x359,
    /* TV Out         */  0x4001        
};

static struct dicom_s dicom_PAL60 =
{
    /* Luma */		  0x4000,
    /* Chroma */	  0x6060,
    /* Frame top */	  0x28,	
    /* Frame bottom */    0x207,
    /* Frame left   */    0x76,
    /* Frame right  */    0x345,
    /* Visible top */	  0x28,	
    /* Visible bottom */  0x207,
    /* Visible left   */  0x76,
    /* Visible right  */  0x345,
    /* TV Out         */  0x4001	
};


static struct dicom_s dicom_NTSC =
{
    0x3a00,	// Luma
    0x6b6b,	// Chroma 
    0x25,	// Frame top
    0x202,     	// Frame bottom
    0x82,	// Frame left   
    0x340,	// Frame right   
    0x25,	// Visible top 	  
    0x202,	// Visible bottom 
    0x82,	// Visible left  
    0x340,	// Visible right  
    0x4001	// TV Out    
};

int em8300_dicom_update(struct em8300_s *em) {
    int ret;
    
    if( (ret=em8300_waitfor(em, ucregister(DICOM_UpdateFlag), 0, 1)) )
	return ret;

    write_ucregister(DICOM_BCSLuma, em->dicom.luma);
    write_ucregister(DICOM_BCSChroma, em->dicom.chroma);
    write_ucregister(DICOM_FrameTop, em->dicom.frametop);
    write_ucregister(DICOM_FrameBottom, em->dicom.framebottom);
    write_ucregister(DICOM_FrameLeft, em->dicom.frameleft);
    write_ucregister(DICOM_FrameRight, em->dicom.frameright);
    write_ucregister(DICOM_VisibleTop, em->dicom.visibletop);
    write_ucregister(DICOM_VisibleBottom, em->dicom.visiblebottom);
    write_ucregister(DICOM_VisibleLeft, em->dicom.visibleleft);
    write_ucregister(DICOM_VisibleRight, em->dicom.visibleright);
    write_ucregister(DICOM_TvOut, em->dicom.tvout);

    if((em->encoder_type == ENCODER_ADV7170) ||
       ((em->encoder_type == ENCODER_ADV7175) &&
       (em->video_mode == EM8300_VIDEOMODE_NTSC)))
        write_ucregister(DICOM_Control,0x9efe);
    
    write_ucregister(DICOM_UpdateFlag,1);
    
    return em8300_waitfor(em, ucregister(DICOM_UpdateFlag), 0, 1);
}

int em8300_dicom_setmode(struct em8300_s *em, int mode) {
    switch(mode) {
	case DICOM_MODE_PAL:
	    em->dicom = dicom_PAL;
	    break;
	case DICOM_MODE_PAL60:
	    em->dicom = dicom_PAL60;
	    break;
	case DICOM_MODE_NTSC:
	    em->dicom = dicom_NTSC;
	    break;
    }
    return em8300_dicom_update(em);
}
