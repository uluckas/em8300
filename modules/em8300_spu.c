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

unsigned default_palette[16] = {
    0xe18080, 0x2b8080, 0x847b9c, 0x51ef5a, 0x7d8080, 0xb48080, 0xa910a5,
    0x6addca,0xd29210,0x1c76b8,0x50505a,0x30b86d,0x5d4792,
    0x3dafa5,0x718947,0xeb8080
};

int em8300_spu_setpalette(struct em8300_s *em, unsigned *pal)
{
    int i,palette;
    
    palette = ucregister(SP_Palette);

    for(i=0; i < 16; i++) {
	write_register(palette+i*2, pal[i] >> 16);
	write_register(palette+i*2+1, pal[i] & 0xffff);
    }
    return 0;
}

void em8300_spu_check_ptsfifo(struct em8300_s *em) {
    int ptsfifoptr;
    
    if(em->sp_ptsfifo_waiting) {
	em->sp_ptsfifo_waiting=0;

	ptsfifoptr = ucregister(SP_PTSFifo) + 2*em->sp_ptsfifo_ptr;

	if(!(read_register(ptsfifoptr+1) & 1))
	    wake_up_interruptible(&em->sp_ptsfifo_wait);
    }
}

int em8300_spu_write(struct em8300_s *em, const char * buf,
		       size_t count, loff_t *ppos)
{
    int flags=0;
    em->sp_ptsvalid=0;
    if(em->sp_ptsvalid) {
	int ptsfifoptr;
	
	em->sp_ptsvalid=0;
	ptsfifoptr = ucregister(SP_PTSFifo) + 2*em->sp_ptsfifo_ptr;
	
	if(read_register(ptsfifoptr+1) & 1) {
	    em->sp_ptsfifo_waiting=1;
	    interruptible_sleep_on(&em->sp_ptsfifo_wait);
	    em->sp_ptsfifo_waiting=0;
	}

	if(signal_pending(current)) 
	    return -EINTR;

	write_register(ptsfifoptr+0, em->sp_pts >> 16);
	write_register(ptsfifoptr+1, (em->sp_pts & 0xffff) | 1);
	em->sp_ptsfifo_ptr++;
	em->sp_ptsfifo_ptr &= read_ucregister(SP_PTSSize)/2 - 1;
    }

    return em8300_fifo_writeblocking(em->spfifo, count, buf, 0, flags);
}

int em8300_spu_ioctl(struct em8300_s *em, unsigned int cmd, unsigned long arg)
{
    int err;
    unsigned clu[16];
    
    switch(cmd) {
    case EM8300_IOCTL_SPU_SETPTS:
	em->sp_pts = (int)arg / 2;
	em->sp_ptsvalid = 1;
	break;
    case EM8300_IOCTL_SPU_SETPALETTE:
	if ((err = verify_area(VERIFY_READ, (void *)arg, 16*4) < 0))
	    return err;	
	copy_from_user(clu,(void *)arg,16*4);
	em8300_spu_setpalette(em,clu);
	break;
    default:
	return -EINVAL;
    }
    return 0;
}

int em8300_spu_init(struct em8300_s *em) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
    init_waitqueue(&em->sp_ptsfifo_wait);
#else
    init_waitqueue_head(&em->sp_ptsfifo_wait);
#endif
    return 0;
}

int em8300_spu_open(struct em8300_s *em)
{
    em->sp_ptsfifo_ptr=0;
    em->sp_ptsvalid = 0;
    em8300_spu_setpalette(em,default_palette);
    write_ucregister(SP_Command, 0x2);

    return 0;
}
