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

#include <linux/soundcard.h>

int em8300_waitfor(struct em8300_s *em, int reg, int val, int mask) {
    int tries;

    for(tries=0; tries < 100 ; tries++) {
	if((em->mem[reg] & mask) == val)
	    return 0;
	udelay(10000);
    }
    return -ETIME;
}

int em8300_setregblock(struct em8300_s *em, int offset, int val, int len) {
    int i;

    val = val | (val << 8) | (val << 16) | (val << 24);
    
    em->mem[0x1c11] = offset & 0xffff;
    em->mem[0x1c12] = (offset >> 16) & 0xffff;
    em->mem[0x1c13] = len;
    em->mem[0x1c14] = len;
    em->mem[0x1c15] = 0;
    em->mem[0x1c16] = 1;
    em->mem[0x1c17] = 1;
    em->mem[0x1c18] = offset & 0xffff;
    em->mem[0x1c19] = (offset >> 16) & 0xffff;

    em->mem[0x1c1a] = 1;

    for(i=0; i < len/4; i++) 
	em->mem[0x11800] = val;
	
    if(em8300_waitfor(em,0x1c1a, 0, 1))
	return -ETIME;
    return 0;
}

int em8300_writeregblock(struct em8300_s *em, int offset, unsigned *buf, int len) {
    int i;

    em->mem[0x1c11] = offset & 0xffff;
    em->mem[0x1c12] = (offset >> 16) & 0xffff;
    em->mem[0x1c13] = len;
    em->mem[0x1c14] = len;
    em->mem[0x1c15] = 0;
    em->mem[0x1c16] = 1;
    em->mem[0x1c17] = 1;
    em->mem[0x1c18] = offset & 0xffff;
    em->mem[0x1c19] = (offset >> 16) & 0xffff;

    em->mem[0x1c1a] = 1;

    for(i=0; i < len/4; i++) 
	em->mem[0x11800] = *buf++;
	
    if(em8300_waitfor(em,0x1c1a, 0, 1))
	return -ETIME;
    return 0;
}
