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

	if(irqstatus & IRQSTATUS_VIDEO_FIFO) 
	    em8300_fifo_check(em->mvfifo);
	
	if(irqstatus & IRQSTATUS_AUDIO_FIFO) 
	    em8300_fifo_check(em->mafifo);

	if(irqstatus & IRQSTATUS_VIDEO_VBL) {
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

    em8300_microcode_t uc;
    em8300_register_t reg;
    
    struct timeval tv;
    char tmpstr[1024];
    int ret,err,len;
    long tdiff,frames,scr;

    switch(subdevice) {
    case EM8300_SUBDEVICE_AUDIO:
	return em8300_audio_ioctl(em,cmd,arg);
    case EM8300_SUBDEVICE_VIDEO:
	return em8300_video_ioctl(em,cmd,arg);
    case EM8300_SUBDEVICE_SUBPICTURE:
	return em8300_spu_ioctl(em,cmd,arg);
    }
    
    if (_IOC_DIR(cmd) != 0) {
	len = _IOC_SIZE(cmd);
	if (len < 1 || len > 65536 || arg == 0)
	    return -EFAULT;
	if (_IOC_DIR(cmd) & _IOC_WRITE)
	    if ((err = verify_area(VERIFY_READ, (void *)arg, len)) < 0)
		return err;
	if (_IOC_DIR(cmd) & _IOC_READ)
	    if ((err = verify_area(VERIFY_WRITE, (void *)arg, len)) < 0)
                                return err;
    }	
    
    switch(_IOC_NR(cmd)) {
   /* Microcode upload */
    case 0:
	copy_from_user(&uc, (void *)arg, sizeof(em8300_microcode_t));

	if((ret=em8300_ucode_upload(em,uc.ucode, uc.ucode_size)))
	    return ret;

	if((ret=em8300_video_setup(em)))
	    return ret;

	if(!(em->mvfifo = em8300_fifo_alloc()))
	    return -ENOMEM;
	
	if(!(em->mafifo = em8300_fifo_alloc()))
	    return -ENOMEM;

	if(!(em->spfifo = em8300_fifo_alloc()))
	    return -ENOMEM;

	em8300_fifo_init(em,em->mvfifo,
			 MV_PCIStart, MV_PCIWrPtr, MV_PCIRdPtr,
			 MV_PCISize, 0x900, FIFOTYPE_VIDEO);

	em8300_fifo_init(em,em->mafifo, 
			 MA_PCIStart, MA_PCIWrPtr, MA_PCIRdPtr, 
			 MA_PCISize, 0x1000, FIFOTYPE_AUDIO); 

	em8300_fifo_init(em,em->spfifo, 
			 SP_PCIStart, SP_PCIWrPtr, SP_PCIRdPtr, 
			 SP_PCISize, 0x1000, FIFOTYPE_VIDEO); 

	if((ret=em8300_audio_setup(em)))
	    return ret;
	
	if(em->encoder)
	    em->encoder->driver->command(em->encoder,ENCODER_CMD_ENABLEOUTPUT,
					 (void *)1);
 	
	em->ucodeloaded = 1;

	printk(KERN_INFO "em8300: Microcode version 0x%02x loaded\n",read_ucregister(MicroCodeVersion));
	
	break;
    /* Write register */
    case 1:
	copy_from_user(&reg, (void *)arg, sizeof(em8300_register_t));
	if(reg.microcode_register)
	    write_ucregister(reg.reg, reg.val);
	else
	    write_register(reg.reg, reg.val);
	break;
    /* Read register */
    case 2:
	copy_from_user(&reg, (void *)arg, sizeof(em8300_register_t));
	if(reg.microcode_register)
	    reg.val = read_ucregister(reg.reg);
	else
	    reg.val = read_register(reg.reg);
	
	copy_to_user((void *)arg,&reg, sizeof(em8300_register_t));
	break;
    /* Get status report */
    case 3:
	frames = (read_ucregister(MV_FrameCntHi) << 16) | read_ucregister(MV_FrameCntLo);
	scr = (read_ucregister(MV_SCRhi) << 16) | read_ucregister(MV_SCRlo);
	
	do_gettimeofday(&tv);
	tdiff = TIMEDIFF(tv,em->last_status_time);
	em->last_status_time = tv;
	sprintf(tmpstr,"Time elapsed: %ld us\nIRQ time period: %ld\nInterrupts : %d\nFrames: %ld\nSCR: %ld\n",
		tdiff, em->irqtimediff,em->irqcount,frames-em->frames,scr-em->scr);
	em->irqcount = 0;
	em->frames = frames;
	em->scr = scr;
	copy_to_user((void *)arg,tmpstr,strlen(tmpstr)+1);
	break;
    default:
	return -EINVAL;
    }
    return 0;
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
    	err = em8300_video_start(em);
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
    
    em8300[card].inuse[subdevice] = 1;

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
	file->f_dentry->d_inode->i_count++;
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
	break;
    case EM8300_SUBDEVICE_SUBPICTURE:
	break;
    }
    
    em->inuse[subdevice]=0;

    clients--;
    printk("em8300_main.o: Releasing device %d, clients:%d\n",subdevice,clients);
    MOD_DEC_USE_COUNT;

    return(0);
}
   

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
static struct file_operations em8300_fops = {
  NULL,
  NULL,
  em8300_io_write,
  NULL,
  NULL,
  em8300_io_ioctl,
  em8300_io_mmap,
  em8300_io_open,
  NULL,
  em8300_io_release,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL };
#else
static struct file_operations em8300_fops = {
  NULL,
  NULL,
  em8300_io_write,
  NULL,
  NULL,
  em8300_io_ioctl,
  em8300_io_mmap,
  em8300_io_open,
  NULL,
  em8300_io_release,
  NULL,
  NULL,
  NULL };
#endif

void cleanup_module(void) {
    release_em8300(em8300_cards);
    unregister_chrdev(EM8300_MAJOR,EM8300_LOGNAME);
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
	
	em8300_i2c_init(em);

	em->linecounter=0;
    }
    if(register_chrdev(EM8300_MAJOR, EM8300_LOGNAME, &em8300_fops)) {
	printk(KERN_ERR "em8300: unable to get major %d\n", EM8300_MAJOR);
	return -EBUSY;
    }
    
    return 0;
}

