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
    0x108080, 0x286def, 0x902235, 0x51ef5a, 0x44ca66, 0xeb8080, 0x44ca66,
    0x6addca,0xd29210,0x44ca66,0xd29210,0x30b86d,0x5d4792,0x3dafa5,
    0x718947,0xeb8080
};

int em8300_spu_setpalette(struct em8300_s *em, unsigned *pal)
{
    int i,palette;
    
    palette = ucregister(SP_Palette);

    for(i=0; i < 16; i++) {
	write_register(palette+i, pal[i] >> 16);
	write_register(palette+i+1, pal[i] & 0xffff);
    }
    return 0;
}

int em8300_spu_write(struct em8300_s *em, const char * buf,
		       size_t count, loff_t *ppos)
{
    int ptsfifoptr=0;;
    if(em->sp_ptsvalid) {
	printk("em8300_spu.o: spu_write %x,%d\n",count,em->sp_pts);

	ptsfifoptr = ucregister(SP_PTSFifo) + 2*em->sp_ptsfifo_ptr;
	if(!(read_register(ptsfifoptr+1) & 1)) {
	    write_register(ptsfifoptr+0, em->sp_offset);
	    write_register(ptsfifoptr+1, em->sp_pts | 1);
	}
	
	em->sp_ptsfifo_ptr++;
	em->sp_ptsfifo_ptr %= read_ucregister(SP_PTSSize) / 2;
	em->sp_ptsvalid=0;
    }

    em->sp_offset += count;
    return em8300_fifo_writeblocking(em->spfifo, count, buf, 0, 0x8040);
}

int em8300_spu_ioctl(struct em8300_s *em, unsigned int cmd, unsigned long arg)
{
    switch(cmd) {
    case EM8300_IOCTL_SPU_SETPTS:
	em->sp_pts = (int)arg;
	em->sp_ptsvalid = 1;
	break;
    default:
	return -EINVAL;
    }
    return 0;
}

int em8300_spu_open(struct em8300_s *em)
{
    em->sp_ptsvalid = 0;
    em->sp_offset = 0;
    em8300_spu_setpalette(em,default_palette);
    write_ucregister(SP_Command, 0x2);
    return 0;
}
