/*
    em8300.c - EM8300 MPEG-2 decoder device driver

    Copyright (C) 2000 Henrik Johansson <henrikjo@post.utfors.se>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/malloc.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/signal.h>
#include <linux/string.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <asm/segment.h>

#include <linux/version.h>
#include <asm/uaccess.h>

#include <linux/i2c-algo-bit.h>

#include "encoder.h"
#include "em8300.h"
#include "em8300_fifo.h"

MODULE_AUTHOR("Henrik Johansson <henrikjo@post.utfors.se>");
MODULE_DESCRIPTION("EM8300 MPEG-2 decoder");
MODULE_PARM(remap,"1-" __MODULE_STRING(EM8300_MAX) "i");

static int em8300_cards,clients;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
static unsigned int remap[EM8300_MAX]={};
#endif
static struct em8300_s em8300[EM8300_MAX];

static
void em8300_irq(int irq, void *dev_id, struct pt_regs * regs)
{
    struct em8300_s *em = (struct em8300_s *)dev_id;
    int irqstatus;
    struct timeval tv;

    irqstatus = read_ucregister(Q_IrqStatus);

    if( irqstatus & 0x8000) {
	write_ucregister(Q_IrqMask,0x0);
	em->mem[0x1ffa] = 2;

	write_ucregister(Q_IrqStatus,0x8000);

	if(irqstatus & IRQSTATUS_VIDEO_FIFO) {
	    em8300_fifo_check(em->mvfifo);
	}
	
	if(irqstatus & IRQSTATUS_AUDIO_FIFO) {
	    em8300_fifo_check(em->mafifo);
	}

	if(irqstatus & IRQSTATUS_VIDEO_VBL) {
	    em8300_video_check_ptsfifo(em);
	    em8300_spu_check_ptsfifo(em);

	    do_gettimeofday(&tv);
	    em->irqtimediff = TIMEDIFF(tv,em->tv);
	    em->tv = tv;
	    em->irqcount++;
	}
	
	write_ucregister(Q_IrqMask,em->irqmask);
	write_ucregister(Q_IrqStatus,0x0000);
    }
}

static
void release_em8300(int max)
{
    struct em8300_s *em;
    int i;

    for (i=0;i<max; i++) {
	em = &em8300[i];

	if(em->encoder)
	    em->encoder->driver->command(em->encoder,ENCODER_CMD_ENABLEOUTPUT,
					 (void *)0);
	
	em8300_i2c_exit(em);

	write_ucregister(Q_IrqMask,0);
	write_ucregister(Q_IrqStatus,0);
	em->mem[0x2000]=0;
	
	em8300_fifo_free(em->mvfifo);
	em8300_fifo_free(em->mafifo);
	em8300_fifo_free(em->spfifo);
	
	/* free it */
	free_irq(em->dev->irq,em);
	
	/* unmap and free memory */
	if (em->mem)
	    iounmap((unsigned *)em->mem);
    }
}

static int find_em8300(void)
{
    struct pci_dev *dev = NULL;
    unsigned char revision;
    struct em8300_s *em;
    int em8300_n=0;
    int result;
     
    if (!pcibios_present()) {
	printk(KERN_DEBUG "em8300: PCI-BIOS not present or not accessible!\n");
	return -1;
    }
    
    while ((dev = pci_find_device(PCI_VENDOR_ID_SIGMADESIGNS,PCI_DEVICE_ID_SIGMADESIGNS_EM8300, dev))) {
	em = &em8300[em8300_n];
	em->dev= dev;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	em->adr = dev->base_address[0];
	if (remap[em8300_n]) {
	    uint dw;

	    if (remap[em8300_n] < 0x1000)
		remap[em8300_n]<<=20;
	    remap[em8300_n] &= PCI_BASE_ADDRESS_MEM_MASK;
	    printk(KERN_INFO "Remapping to : 0x%08x.\n", remap[em8300_n]);
	    remap[em8300_n] |= em->adr&(~PCI_BASE_ADDRESS_MEM_MASK);
	    pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, remap[em8300_n]);
	    /* commit to PCI bus */
	    pci_read_config_dword(dev,  PCI_BASE_ADDRESS_0, &dw);
	    dev->base_address[0] = em->adr;
	}
	em->adr &= PCI_BASE_ADDRESS_MEM_MASK;
#else
	em->adr = dev->resource[0].start;
#endif	
	em->memsize = 1024*1024;
	
	pci_read_config_byte(dev, PCI_CLASS_REVISION, &revision);
	em->pci_revision = revision;
	printk(KERN_INFO "em8300: EM8300 %x (rev %d) ",
	       dev->device, revision);
	printk("bus: %d, devfn: %d, irq: %d, ",
	       dev->bus->number, dev->devfn, dev->irq);
	printk("memory: 0x%08lx.\n", em->adr);
	
	em->mem = ioremap(em->adr, em->memsize);
	DEBUG(printk(KERN_DEBUG "em8300: mapped-memory at 0x%p\n",em->mem));

	result = request_irq(dev->irq, em8300_irq,
			     SA_SHIRQ|SA_INTERRUPT,"em8300",(void *)em);
	
	if (result==-EINVAL) {
	    printk(KERN_ERR "em8300: Bad irq number or handler\n");
	    return -EINVAL;
	}	
	
	pci_set_master(dev);
	
	em8300_n++;
    }

    if(em8300_n)
	printk(KERN_INFO "em8300: %d EM8300 card(s) found.\n",em8300_n);
  
    return em8300_n;
}

static
int em8300_io_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
    struct em8300_s *em = filp->private_data;
    int subdevice = MINOR(inode->i_rdev) & 3;

    switch(subdevice) {
    case EM8300_SUBDEVICE_AUDIO:
	return em8300_audio_ioctl(em,cmd,arg);
    case EM8300_SUBDEVICE_VIDEO:
	return em8300_video_ioctl(em,cmd,arg);
    case EM8300_SUBDEVICE_SUBPICTURE:
	return em8300_spu_ioctl(em,cmd,arg);
    case EM8300_SUBDEVICE_CONTROL:
	return em8300_control_ioctl(em,cmd,arg);
    }
    return -EINVAL;
}

static
int em8300_io_open(struct inode* inode, struct file* filp) 
{
    int card = MINOR(inode->i_rdev)/4;
    int subdevice = MINOR(inode->i_rdev) & 3;
    struct em8300_s *em = &em8300[card];
    int err=0;
  
    if(card >= em8300_cards)
	return -ENODEV;

    if(subdevice != EM8300_SUBDEVICE_CONTROL)
	if(em8300[card].inuse[subdevice])
	    return -EBUSY;
  
    filp->private_data = &em8300[card];

    switch(subdevice) {
    case EM8300_SUBDEVICE_CONTROL:
	break;
    case EM8300_SUBDEVICE_AUDIO:
	err =  em8300_audio_open(em);
	break;
    case EM8300_SUBDEVICE_VIDEO:
	if(!em->ucodeloaded)
	    return -ENODEV;
	em8300_video_open(em);

	em8300_ioctl_enable_videoout(em,1);

    	em8300_video_setplaymode(em,EM8300_PLAYMODE_PLAY);
	break;
    case EM8300_SUBDEVICE_SUBPICTURE:
	if(!em->ucodeloaded)
	    return -ENODEV;
	err = em8300_spu_open(em);
	break;
    default:
	return -ENODEV;
	break;
    }

    if(err)
	return err;
    
    em8300[card].inuse[subdevice]++;

    clients++;
    printk("em8300_main.o: Opening device %d, Clients:%d\n",subdevice,clients);
    MOD_INC_USE_COUNT;
  
    return(0);
}

static
int em8300_io_write(struct file *file, const char * buf,
		    size_t count, loff_t *ppos)
{
    struct em8300_s *em = file->private_data;
    int subdevice = MINOR(file->f_dentry->d_inode->i_rdev) & 3;
    
    switch (subdevice) {
    case EM8300_SUBDEVICE_VIDEO:
	return em8300_video_write(em,buf,count,ppos);
	break;
    case EM8300_SUBDEVICE_AUDIO:
	return em8300_audio_write(em,buf,count,ppos);
	break;
    case EM8300_SUBDEVICE_SUBPICTURE:
	return em8300_spu_write(em,buf,count,ppos);
	break;
    default:
	return -EPERM;
    }

    
}

int em8300_io_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct em8300_s *em = file->private_data;
    unsigned long size = vma->vm_end - vma->vm_start;
    int subdevice = MINOR(file->f_dentry->d_inode->i_rdev) & 3;

    if(subdevice != EM8300_SUBDEVICE_CONTROL)
	return -EPERM;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
    switch(vma->vm_offset) {
#else	
    switch(vma->vm_pgoff) {
#endif	
    case 0:
	if(size > em->memsize)
	    return -EINVAL;
	
	remap_page_range(vma->vm_start,
			 em->adr,
			 vma->vm_end - vma->vm_start,
			 vma->vm_page_prot);
	vma->vm_file = file;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
	file->f_dentry->d_inode->i_count++;
#else    
	atomic_inc(&file->f_dentry->d_inode->i_count);
#endif   
	break;
    default:
	return -EINVAL;
    }
    return 0;
}

int em8300_io_release(struct inode* inode, struct file* filp) 
{
    struct em8300_s *em = filp->private_data;
    int subdevice = MINOR(inode->i_rdev) & 3;
    
    switch(subdevice) {
    case EM8300_SUBDEVICE_AUDIO:
	em8300_audio_release(em);
	break;
    case EM8300_SUBDEVICE_VIDEO:
	em8300_video_release(em);
	em8300_ioctl_enable_videoout(em,0);	
	break;
    case EM8300_SUBDEVICE_SUBPICTURE:
	break;
    }
    
    em->inuse[subdevice]--;

    clients--;
    printk("em8300_main.o: Releasing device %d, clients:%d\n",subdevice,clients);
    MOD_DEC_USE_COUNT;

    return(0);
}
   
static struct file_operations em8300_fops = {
  write: em8300_io_write,
  ioctl: em8300_io_ioctl,
  mmap: em8300_io_mmap,
  open: em8300_io_open,
  release: em8300_io_release,
};

void cleanup_module(void) {
    release_em8300(em8300_cards);
    unregister_chrdev(EM8300_MAJOR,EM8300_LOGNAME);
}

int em8300_init(struct em8300_s *em) {
    /* Setup parameters */
    em->videodelay = 0x3000 / 2;
    em->max_videodelay = 90000*4;    
    em->audio_autosync = 0;
    
    write_register(0x30000, read_register(0x30000));

    write_register(0x1f50, 0x123);

    if(read_register(0x1f50) == 0x123) {
       em->chip_revision = 2;
       if (0x40 & read_register(0x1c08)) {
          em->var_video_value = 0xd34;
	  em->mystery_divisor = 0x107ac;
	  em->var_ucode_reg2 = 0x272;
	  em->var_ucode_reg3 = 0x8272;
	  if (0x20 & read_register(0x1c08)) {
#ifdef EM8300_USE_BT865
	      em->var_ucode_reg1 = 0x800;
#else
	      em->var_ucode_reg1 = 0x818;
#endif
	  }
       } else {
	  em->var_video_value = 0xce4;
	  em->mystery_divisor = 0x101d0;
	  em->var_ucode_reg2 = 0x25a;
          em->var_ucode_reg3 = 0x825a;
       }
    } else {
       em->chip_revision = 1;            
       em->var_ucode_reg1 = 0x80;
       em->var_video_value = 0xce4;
       em->mystery_divisor = 0x101d0;
       em->var_ucode_reg2 = 0xC7;
       em->var_ucode_reg3 = 0x8c7;
    }

    printk("em8300_main.o: Chip revision: %d\n",em->chip_revision);
    
    em8300_i2c_init(em);

    return 0;
}

int init_module(void)
{
    int card;
    struct em8300_s *em;

    memset(&em8300, 0, sizeof(em8300));
    
    /* Find EM8300 cards */
    em8300_cards = find_em8300();

    /* Initialize EM8300 cards */
    for(card = 0; card < em8300_cards; card++) {
	em = &em8300[card];

	em->irqmask = 0;
	
	em->encoder = NULL;
	em->eeprom = NULL;

	em->linecounter=0;

	em8300_init(em);
    }
    if(register_chrdev(EM8300_MAJOR, EM8300_LOGNAME, &em8300_fops)) {
	printk(KERN_ERR "em8300: unable to get major %d\n", EM8300_MAJOR);
	return -EBUSY;
    }
    
    return 0;
}

