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

    em->video_first = 1;
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
    
    write_register(0x1f47,0x0);
    write_register(0x1f5e,0x9afe);
    write_ucregister(DICOM_Control,0x9afe);

    write_register(0x1f4d,0x3c3c);
    write_register(0x1f4e,0x3c00);
    write_register(0x1f4e,0x3c3c);

    write_register(0x1f4d,0x808);
    write_register(0x1f4d,0x1010);
		   
    em9010_write(em,7,0x40);
    em9010_write(em,9,0x4);

    em9010_read(em,0);
    
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

    em9010_write16(em,0x8,0xff);
    em9010_write16(em,0x10,0xff);
    em9010_write16(em,0x20,0xff);

    em9010_write(em,0xa,0x0);


    /* FIXME: Detect loopback cable here */
    DEBUG(printk("em8300: overlay reg 0x80 = %x \n",em9010_read16(em,0x80)));

    em9010_write(em,0xb,0xc8);

    DEBUG(printk("em8300: register 0x1f4b = %x (0x138)\n",read_register(0x1f4b)));

    em9010_write16(em,1,0x4fe);
    em9010_write(em,1,4);
    em9010_write(em,5,0);
    em9010_write(em,6,0);
    em9010_write(em,7,0x40);
    em9010_write(em,8,0x80);
    em9010_write(em,0xc,0x8c);
    em9010_write(em,9,0);

    write_ucregister(DICOM_Kmin,0x447);

    em9010_write(em,7,0x80);
    em9010_write(em,9,0);
    
    write_register(0x1f47,0x18);
    write_register(0x1f5e,0x9afe);
    write_ucregister(DICOM_Control,0x9afe);

    udelay(100);
    
    write_ucregister(ForcedLeftParity,0x2);
    write_ucregister(MV_Threshold,0x90);
    write_register(0x1ffa,0x2);
    write_ucregister(Q_IrqMask,0x0);
    write_ucregister(Q_IrqStatus,0x0);
    write_ucregister(Q_IntCnt,0x64);

    if(read_register(0x1c08) & 0x40)
	write_register(0x1ffb,0xd34);
    else
	write_register(0x1ffb,0xce4);

    write_ucregister(MA_Threshold,0x8);

    /* Release reset */
    write_register(0x2000,0x1);

    if(mpegvideo_command(em,0x11)) {
	DEBUG( printk("em8300: mpegvideo_command(0x11) failed\n"));
	return -ETIME;
    }

    DEBUG(printk("DICOM display data: 0x%x\n",read_ucregister(DICOM_Display_Data)));

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
    em8300_ioctl_setaspectratio(em,EM8300_ASPECTRATIO_3_2);

    em8300_dicom_setBCS(em, 500,500,500);

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

void em8300_video_check_ptsfifo(struct em8300_s *em) {
    int ptsfifoptr;
    
    if(em->video_ptsfifo_waiting) {
	em->video_ptsfifo_waiting=0;

	ptsfifoptr = ucregister(MV_PTSFifo) + 2*em->video_ptsfifo_ptr;

	if(!(read_register(ptsfifoptr+1) & 1))
	    wake_up_interruptible(&em->video_ptsfifo_wait);
    }
}

int em8300_video_write(struct em8300_s *em, const char * buf,
		       size_t count, loff_t *ppos)
{
    unsigned flags=0;
    long scrptsdiff,scr; 
    //    em->video_ptsvalid=0;
    if(em->video_ptsvalid) {
	int ptsfifoptr=0;
	em->video_pts>>=1;
	//	printk("em8300_video.o: video_write %x,%x %x\n",count,em->video_pts,em->video_offset);
	scr = read_ucregister(MV_SCRlo) | (read_ucregister(MV_SCRhi) << 16);
	scrptsdiff = em->video_pts+em->videodelay-scr;
	if((scrptsdiff > 90000) || (scrptsdiff < -90000)) {
	    unsigned startpts;

	    em8300_fifo_sync(em->mvfifo);	    
	    
	    startpts = em->video_pts + em->videodelay;
	    write_ucregister(MV_SCRlo, startpts & 0xffff);
	    write_ucregister(MV_SCRhi, (startpts >> 16) & 0xffff);
	    printk("Setting SCR: %d\n",startpts);
	} 

	flags = 0x40000000;
	ptsfifoptr = ucregister(MV_PTSFifo) + 4*em->video_ptsfifo_ptr;

	if(read_register(ptsfifoptr+1) & 1) {
	    em->video_ptsfifo_waiting=1;
	    interruptible_sleep_on(&em->video_ptsfifo_wait);
	    em->video_ptsfifo_waiting=0;
	    if(signal_pending(current)) 
		return -EINTR;
	}	
	
	write_register(ptsfifoptr, em->video_offset >> 16);
	write_register(ptsfifoptr+1, em->video_offset & 0xffff);
	write_register(ptsfifoptr+2, em->video_pts >> 16);
	write_register(ptsfifoptr+3, (em->video_pts & 0xffff) | 1);
	
	em->video_ptsfifo_ptr++;
	em->video_ptsfifo_ptr %= read_ucregister(MV_PTSSize) / 4;

	em->video_ptsvalid=0;
    }

    em->video_offset += count;
    return em8300_fifo_writeblocking(em->mvfifo, count, buf,flags);
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
