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
#include "em8300_fifo.h"

#include <linux/soundcard.h>

static
int mpegvideo_command(struct em8300_s *em, int cmd) {
    if(read_ucregister(MV_Command) != 0xffff)
	return -1;

    write_ucregister(MV_Command, cmd);

    return em8300_waitfor(em,ucregister(MV_Command), 0xffff, 0xffff);
}

int em8300_video_stop(struct em8300_s *em) {
    em->irqmask &= ~IRQSTATUS_VIDEO_FIFO;
    write_ucregister(Q_IrqMask, em->irqmask);
    return mpegvideo_command(em,MVCOMMAND_STOP);
}

int em8300_video_start(struct em8300_s *em) {
    em->irqmask |= IRQSTATUS_VIDEO_FIFO | IRQSTATUS_VIDEO_VBL;
    write_ucregister(Q_IrqMask, em->irqmask);

    if(mpegvideo_command(em,0x11))
	return -ETIME;

    if(mpegvideo_command(em,0x10))
	return -ETIME;

    em8300_video_setspeed(em,0x900);

    write_ucregister(MV_FrameEventLo,0xffff);
    write_ucregister(MV_FrameEventHi,0x7fff);

    return mpegvideo_command(em,MVCOMMAND_START);
}

int em8300_video_sync(struct em8300_s *em) {
    int rdptr,wrptr;
    
    do {
	wrptr =
	    read_ucregister(MV_Wrptr_Lo) |
	    (read_ucregister(MV_Wrptr_Hi)<<16);
	rdptr =
	    read_ucregister(MV_RdPtr_Lo) |  
	    (read_ucregister(MV_RdPtr_Hi)<<16);
	if(rdptr != wrptr)
	    schedule_timeout(HZ/10);
	if(signal_pending(current)) {
	    printk("em8300.o: FIFO sync interrupted\n");
	    return -EINTR;
	}
    } while(rdptr != wrptr);

    return 0;
}

int em8300_video_setup(struct em8300_s *em) {
    int displaybuffer;
    int val;
    
    em->clockgen = 0xb | CLOCKGEN_SAMPFREQ_48;
    
    em8300_clockgen_write(em,em->clockgen);
    
    write_register(0x1f47,0x0);
    write_register(0x1f5e,0x9afe);

    write_ucregister(DICOM_Control,0x9afe);

    udelay(100);

    write_ucregister(DICOM_UpdateFlag,0x0);
    write_ucregister(DICOM_VisibleLeft,0x168);
    write_ucregister(DICOM_VisibleTop,0x2e);
    write_ucregister(DICOM_VisibleRight,0x36b);
    write_ucregister(DICOM_VisibleBottom,0x11e);
    write_ucregister(DICOM_FrameLeft,0x168);
    write_ucregister(DICOM_FrameTop,0x2e);
    write_ucregister(DICOM_FrameRight,0x36b);
    write_ucregister(DICOM_FrameBottom,0x11e);
    write_ucregister(DICOM_TvOut,0x4000);
    write_ucregister(DICOM_UpdateFlag,0x1);

    DEBUG(printk("em8300: register 0x1f4b = %x (0x138)\n",read_register(0x1f4b)));
	  
    write_ucregister(DICOM_Kmin,0x455);
    write_register(0x1f47,0x18);
    write_register(0x1f5e,0x9afe);
    write_ucregister(DICOM_Control,0x9afe);

    udelay(100);
    
    write_ucregister(ForcedLeftParity,0x2);
    write_ucregister(MV_Threshold,0x50);
    write_register(0x1ffa,0x2);
    write_ucregister(Q_IrqMask,0x0);
    write_ucregister(Q_IrqStatus,0x0);
    write_ucregister(Q_IntCnt,0x64);
    write_register(0x1ffb,0xce4);
    write_ucregister(MA_Threshold,0x8);

    /* Release reset */
    write_register(0x2000,0x1);

    if(mpegvideo_command(em,0x11)) {
	DEBUG( printk("em8300: mpegvideo_command(0x11) failed\n"));
	return -ETIME;
    }

    displaybuffer = read_ucregister(DICOM_DisplayBuffer)+0x1000;

    val = read_register(displaybuffer);
    DEBUG(printk("DICOM buffer: 0x%x(%d)\n",val,val));

    val = read_register(displaybuffer+1);
    DEBUG(printk("              0x%x(%d)\n",val,val));

    val = read_register(displaybuffer+2) & 0x1fff;
    DEBUG(printk("              0x%x(%d)\n",val,val));

    val = read_register(displaybuffer+3);
    val = (read_register(displaybuffer+4) << 16) | val;
    val <<= 4;
    DEBUG(printk("              0x%x\n",val << 4));

    val = read_register(displaybuffer+5);
    val |= read_register(displaybuffer+6) << 16;
    val <<= 4;
    DEBUG(printk("              0x%x(%d)\n",val,val));
    

    write_ucregister(SP_Status,0x0);
    
    if(mpegvideo_command(em,0x10)) {
	DEBUG( printk("em8300: mpegvideo_command(0x10) failed\n"));
	return -ETIME;
    }

    em8300_video_setspeed(em,0x900);

    write_ucregister(MV_FrameEventLo,0xffff);
    write_ucregister(MV_FrameEventHi,0x7fff);

    em8300_ioctl_setvideomode(em,EM8300_VIDEOMODE_DEFAULT);

    /*if(em->encoder_type == ENCODER_ADV7170)
      em->dicom.visibleright = 0x759;*/

    if(em8300_dicom_update(em)) {
	DEBUG( printk("em8300: DICOM Update failed\n"));
	return -ETIME;
    }
    
    return 0;
}


void em8300_video_setspeed(struct em8300_s *em, int speed)
{
    if(read_ucregister(MicroCodeVersion) >= 0x29)
	write_ucregister(MV_SCRSpeed, speed);
    else {
	write_ucregister(MV_SCRSpeed, speed >> 8);
    }
}

int em8300_video_write(struct em8300_s *em, const char * buf,
		       size_t count, loff_t *ppos)
{
    unsigned flags=0;
    //    em->video_ptsvalid=0;
    if(em->video_ptsvalid) {
	int ptsfifoptr=0;
	//	printk("em8300_video.o: video_write %x,%d\n",count,em->video_pts);
	flags = 0x40000000;
	em->video_pts>>=1;
	
	ptsfifoptr = ucregister(MV_PTSFifo) + 4*em->video_ptsfifo_ptr;
	if(!(read_register(ptsfifoptr+3) & 1)) {
	    write_register(ptsfifoptr, em->video_offset >> 16);
	    write_register(ptsfifoptr+1, em->video_offset & 0xffff);
	    write_register(ptsfifoptr+2, em->video_pts >> 16);
	    write_register(ptsfifoptr+3, (em->video_pts & 0xffff) | 1);
	} else {
	    printk("em8300_video.o: PTS Fifo full\n");
	}
	
	em->video_ptsfifo_ptr++;
	em->video_ptsfifo_ptr %= read_ucregister(MV_PTSSize) / 4;
	em->video_ptsvalid=0;
    }

    em->video_offset += count;
    return em8300_fifo_writeblocking(em->mvfifo, count, buf,0,flags);
}

int em8300_video_ioctl(struct em8300_s *em, unsigned int cmd, unsigned long arg)
{
    switch(cmd) {
    case EM8300_IOCTL_VIDEO_SETPTS:
	em->video_pts = (int)arg;
	em->video_ptsvalid = 1;
	break;
    default:
	return -EINVAL;
    }
    return 0;
}

int em8300_video_release(struct em8300_s *em)
{
    em->video_ptsfifo_ptr=0;
    em->video_offset=0;
    em->video_ptsvalid=0;
    em8300_fifo_sync(em->mvfifo);
    em8300_video_sync(em);
    return em8300_video_stop(em);    
}
