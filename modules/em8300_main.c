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
#include <linux/config.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/sound.h>
#include <linux/signal.h>
#include <linux/string.h>
#include <linux/time.h>
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif
#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <asm/segment.h>
#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

#include <linux/version.h>
#include <asm/uaccess.h>

#include <linux/i2c-algo-bit.h>

#include "encoder.h"

#include "em8300_reg.h"
#include <linux/em8300.h>
#include "em8300_fifo.h"
#include "em8300_version.h"

/* It seems devfs will implement a new scheme of enumerating minor numbers.
 * Currently it seems broken. But that is why we added these macros.
 */
#define EM8300_MINOR(inode) ((inode)->i_rdev % 4)
#define EM8300_CARD(inode) ((inode)->i_rdev / 4)

#ifndef I2C_BITBANGING
#error "This needs the I2C Bit Banging Interface in your Kernel"
#endif

#ifdef MODULE
MODULE_AUTHOR("Henrik Johansson <henrikjo@post.utfors.se>");
MODULE_DESCRIPTION("EM8300 MPEG-2 decoder");
MODULE_SUPPORTED_DEVICE("em8300");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
MODULE_PARM(remap,"1-" __MODULE_STRING(EM8300_MAX) "i");
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,9)
MODULE_LICENSE("GPL");
#endif
#endif

static unsigned int use_bt865[EM8300_MAX]={};
MODULE_PARM(use_bt865, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(use_bt865, "Set this to 1 if you have a bt865. It changes some internal register values. Defaults to 0.");

/*
 * Module params by Jonas Birmé (birme@jpl.nu)
 */
int dicom_other_pal = 1;
MODULE_PARM(dicom_other_pal, "i");
MODULE_PARM_DESC(dicom_other_pal, "If this is set, then some internal register values are swapped for PAL and NTSC. Defaults to 1.");

int dicom_fix = 1;
MODULE_PARM(dicom_fix, "i");
MODULE_PARM_DESC(dicom_fix, "If this is set then some internal register values are changed. Fixes green screen problems for some. Defaults to 1.");

int dicom_control = 1;
MODULE_PARM(dicom_control, "i");
MODULE_PARM_DESC(dicom_control, "If this is set then some internal register values are changed. Fixes green screen problems for some. Defaults to 1.");

int bt865_ucode_timeout = 0;
MODULE_PARM(bt865_ucode_timeout, "i");
MODULE_PARM_DESC(bt865_ucode_timeout, "Set this to 1 if you have a bt865 and get timeouts when uploading the microcode. Defaults to 0.");

int activate_loopback = 0;
MODULE_PARM(activate_loopback, "i");
MODULE_PARM_DESC(activate_loopback, "If you lose video after loading the modules or uploading the microcode set this to 1. Defaults to 0.");

static int em8300_cards,clients;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
static unsigned int remap[EM8300_MAX]={};
#endif
static struct em8300_s em8300[EM8300_MAX];
#ifdef REGISTER_DSP
static int dsp_num_table[16];
#endif
#ifdef CONFIG_DEVFS_FS
static int em8300_major;
devfs_handle_t em8300_handle[EM8300_MAX*4];
#endif
#ifdef CONFIG_PROC_FS
struct proc_dir_entry *em8300_proc;
#endif

static void em8300_irq(int irq, void *dev_id, struct pt_regs * regs)
{
	struct em8300_s *em = (struct em8300_s *) dev_id;
	int irqstatus;
	struct timeval tv;

	irqstatus = read_ucregister(Q_IrqStatus);

	if (irqstatus & 0x8000) {
		write_ucregister(Q_IrqMask, 0x0);
		em->mem[EM8300_INTERRUPT_ACK] = 2;

		write_ucregister(Q_IrqStatus, 0x8000);

		if (irqstatus & IRQSTATUS_VIDEO_FIFO) {
			em8300_fifo_check(em->mvfifo);
		}
	
		if (irqstatus & IRQSTATUS_AUDIO_FIFO) {
			em8300_fifo_check(em->mafifo);
		}

		if (irqstatus & IRQSTATUS_VIDEO_VBL) {
			em8300_video_check_ptsfifo(em);
			em8300_spu_check_ptsfifo(em);

			do_gettimeofday(&tv);
			em->irqtimediff = TIMEDIFF(tv, em->tv);
			em->tv = tv;
			em->irqcount++;
		}
	
		write_ucregister(Q_IrqMask, em->irqmask);
		write_ucregister(Q_IrqStatus, 0x0000);
	}
}

static void release_em8300(int max)
{
	struct em8300_s *em;
	int i;

	for (i = 0; i < max; i++) {
		em = &em8300[i];

		if(em->encoder) {
			em->encoder->driver->command(em->encoder, ENCODER_CMD_ENABLEOUTPUT, (void *) 0);
		}
	
#ifdef CONFIG_MTRR	
		if (em->mtrr_reg) {
			mtrr_del(em->mtrr_reg,em->adr, em->memsize);
		}
#endif
		
		em8300_i2c_exit(em);

		write_ucregister(Q_IrqMask, 0);
		write_ucregister(Q_IrqStatus, 0);
		em->mem[0x2000] = 0;
	
		em8300_fifo_free(em->mvfifo);
		em8300_fifo_free(em->mafifo);
		em8300_fifo_free(em->spfifo);
	
		/* free it */
		free_irq(em->dev->irq, em);
	
		/* unmap and free memory */
		if (em->mem) {
			iounmap((unsigned *) em->mem);
		}
	}
}

static int find_em8300(void)
{
	struct pci_dev *dev = NULL;
	unsigned char revision;
	struct em8300_s *em;
	int em8300_n = 0;
	int result;
	 
	if (!pcibios_present()) {
		printk(KERN_ERR "em8300: PCI-BIOS not present or not accessible!\n");
		return -1;
	}
	
	while ((dev = pci_find_device(PCI_VENDOR_ID_SIGMADESIGNS, PCI_DEVICE_ID_SIGMADESIGNS_EM8300, dev))) {
		em = &em8300[em8300_n];
		em->dev = dev;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
		em->adr = dev->base_address[0];
		if (remap[em8300_n]) {
			uint dw;

			if (remap[em8300_n] < 0x1000) {
				remap[em8300_n] <<= 20;
			}
			remap[em8300_n] &= PCI_BASE_ADDRESS_MEM_MASK;
			pr_info("Remapping to : 0x%08x.\n", remap[em8300_n]);
			remap[em8300_n] |= em->adr&(~PCI_BASE_ADDRESS_MEM_MASK);
			pci_write_config_dword(dev, PCI_BASE_ADDRESS_0, remap[em8300_n]);
			/* commit to PCI bus */
			pci_read_config_dword(dev, PCI_BASE_ADDRESS_0, &dw);
			dev->base_address[0] = em->adr;
		}
		em->adr &= PCI_BASE_ADDRESS_MEM_MASK;
#else
		em->adr = dev->resource[0].start;
#endif	
		em->memsize = 1024 * 1024;
	
		pci_read_config_byte(dev, PCI_CLASS_REVISION, &revision);
		em->pci_revision = revision;
		pr_info("em8300: EM8300 %x (rev %d) ", dev->device, revision);
		printk("bus: %d, devfn: %d, irq: %d, ", dev->bus->number, dev->devfn, dev->irq);
		printk("memory: 0x%08lx.\n", em->adr);
	
		em->mem = ioremap(em->adr, em->memsize);
		pr_info("em8300: mapped-memory at 0x%p\n", em->mem);
#ifdef CONFIG_MTRR
		em->mtrr_reg = mtrr_add(em->adr, em->memsize, MTRR_TYPE_UNCACHABLE, 1);
		if (em->mtrr_reg) pr_info("em8300: using MTRR\n");
#endif

		result = request_irq(dev->irq, em8300_irq, SA_SHIRQ | SA_INTERRUPT, "em8300", (void *) em);
	
		if (result == -EINVAL) {
			printk(KERN_ERR "em8300: Bad irq number or handler\n");
			return -EINVAL;
		}	
	
		pci_set_master(dev);
	
		em8300_n++;
	}

	if (em8300_n) {
		pr_info("em8300: %d EM8300 card(s) found.\n", em8300_n);
	}
  
	return em8300_n;
}

static int em8300_io_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
	struct em8300_s *em = filp->private_data;
	int subdevice = EM8300_MINOR(inode) % 4;

	switch (subdevice) {
	case EM8300_SUBDEVICE_AUDIO:
		return em8300_audio_ioctl(em, cmd, arg);
	case EM8300_SUBDEVICE_VIDEO:
		return em8300_video_ioctl(em, cmd, arg);
	case EM8300_SUBDEVICE_SUBPICTURE:
		return em8300_spu_ioctl(em, cmd, arg);
	case EM8300_SUBDEVICE_CONTROL:
		return em8300_control_ioctl(em, cmd, arg);
	}

	return -EINVAL;
}

static int em8300_io_open(struct inode* inode, struct file* filp) 
{
	int card = EM8300_MINOR(inode) / 4;
	int subdevice = EM8300_MINOR(inode) % 4;
	struct em8300_s *em = &em8300[card];
	int err = 0;
  
	if (card >= em8300_cards) {
		return -ENODEV;
	}

	if (subdevice != EM8300_SUBDEVICE_CONTROL) {
		if (em8300[card].inuse[subdevice]) {
			return -EBUSY;
		}
	}
  
	filp->private_data = &em8300[card];

	switch (subdevice) {
	case EM8300_SUBDEVICE_CONTROL:
		em8300[card].nonblock[0] = (filp->f_flags == O_NONBLOCK);
		break;
	case EM8300_SUBDEVICE_AUDIO:
		em8300[card].nonblock[1] = (filp->f_flags == O_NONBLOCK);
		err = em8300_audio_open(em);
		break;
	case EM8300_SUBDEVICE_VIDEO:
		em8300[card].nonblock[2] = (filp->f_flags == O_NONBLOCK);
		if (!em->ucodeloaded) {
			return -ENODEV;
		}
		em8300_video_open(em);

		em8300_ioctl_enable_videoout(em, 1);

		em8300_video_setplaymode(em, EM8300_PLAYMODE_PLAY);
		break;
	case EM8300_SUBDEVICE_SUBPICTURE:
		em8300[card].nonblock[3] = (filp->f_flags == O_NONBLOCK);
		if (!em->ucodeloaded) {
			return -ENODEV;
		}
		err = em8300_spu_open(em);
		break;
	default:
		return -ENODEV;
		break;
	}

	if (err) {
		return err;
	}
	
	em8300[card].inuse[subdevice]++;

	clients++;
	pr_info("em8300_main.o: Opening device %d, Clients:%d\n", subdevice, clients);
	MOD_INC_USE_COUNT;
  
	return(0);
}

static int em8300_io_write(struct file *file, const char * buf,	size_t count, loff_t *ppos)
{
	struct em8300_s *em = file->private_data;
	int subdevice = EM8300_MINOR(file->f_dentry->d_inode) % 4;
	
	switch (subdevice) {
	case EM8300_SUBDEVICE_VIDEO:
		return em8300_video_write(em, buf, count, ppos);
		break;
	case EM8300_SUBDEVICE_AUDIO:
		return em8300_audio_write(em, buf, count, ppos);
		break;
	case EM8300_SUBDEVICE_SUBPICTURE:
		return em8300_spu_write(em, buf, count, ppos);
		break;
	default:
		return -EPERM;
	}
}

int em8300_io_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct em8300_s *em = file->private_data;
	unsigned long size = vma->vm_end - vma->vm_start;
	int subdevice = EM8300_MINOR(file->f_dentry->d_inode) % 4;

	if (subdevice != EM8300_SUBDEVICE_CONTROL) {
		return -EPERM;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	switch ((unsigned long) vma->vm_offset) {
#else	
	switch (vma->vm_pgoff) {
#endif	
	case 0:
		if (size > em->memsize) {
			return -EINVAL;
		}
	
		remap_page_range(vma->vm_start, em->adr, vma->vm_end - vma->vm_start, vma->vm_page_prot);
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
	int subdevice = EM8300_MINOR(inode) % 4;
	
	switch (subdevice) {
	case EM8300_SUBDEVICE_AUDIO:
		em8300_audio_release(em);
		break;
	case EM8300_SUBDEVICE_VIDEO:
		em8300_video_release(em);
		em8300_ioctl_enable_videoout(em, 0);	
		break;
	case EM8300_SUBDEVICE_SUBPICTURE:
		break;
	}
	
	em->inuse[subdevice]--;

	clients--;
	pr_info("em8300_main.o: Releasing device %d, clients:%d\n", subdevice, clients);
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

#ifdef REGISTER_DSP
static int em8300_dsp_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
	struct em8300_s *em = filp->private_data;
	return em8300_audio_ioctl(em, cmd, arg);
}

static int em8300_dsp_open(struct inode* inode, struct file* filp) 
{
	int dsp_num = ((MINOR(inode->i_rdev) >> 4) & 0x0f);
	int card = dsp_num_table[dsp_num] - 1;
	int err=0;

        pr_debug("em8300: opening dsp %i for card %i\n", dsp_num, card);

	if (card < 0 || card >= em8300_cards) {
		return -ENODEV;
	}

        if (em8300[card].inuse[EM8300_SUBDEVICE_AUDIO]) {
		return -EBUSY;
	}
  
	filp->private_data = &em8300[card];

        err = em8300_audio_open(&em8300[card]);

	if (err) {
		return err;
	}
	
	em8300[card].inuse[EM8300_SUBDEVICE_AUDIO]++;

	clients++;
	pr_info("em8300_main.o: Opening device %d, Clients:%d\n", EM8300_SUBDEVICE_AUDIO, clients);
	MOD_INC_USE_COUNT;
  
	return(0);
}

static int em8300_dsp_write(struct file *file, const char * buf, size_t count, loff_t *ppos)
{
	struct em8300_s *em = file->private_data;
	return em8300_audio_write(em, buf, count, ppos);
}

int em8300_dsp_release(struct inode* inode, struct file* filp) 
{
	struct em8300_s *em = filp->private_data;
	
	em8300_audio_release(em);
	
	em->inuse[EM8300_SUBDEVICE_AUDIO]--;

	clients--;
	pr_info("em8300_main.o: Releasing device %d, clients:%d\n", EM8300_SUBDEVICE_AUDIO, clients);
	MOD_DEC_USE_COUNT;

	return(0);
}

static struct file_operations em8300_dsp_audio_fops = {
	write: em8300_dsp_write,
	ioctl: em8300_dsp_ioctl,
	open: em8300_dsp_open,
	release: em8300_dsp_release,
};
#endif

#ifdef CONFIG_PROC_FS
int em8300_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
        int len = 0;
	struct em8300_s *em = (struct em8300_s *) data;    
    
        *start = 0;
        *eof = 1;
    
        len += sprintf(page + len, "----- Driver Info -----\n");
	len += sprintf(page + len, "em8300 module version %s\n", MODULE_VERSION);
	len += sprintf(page + len, "Compiled with %s\n", COMPILER_VERSION);
        if (em->ucodeloaded) {
		/* Device information */
		len += sprintf(page + len, "Card revision %d\n", em->pci_revision);
		len += sprintf(page + len, "Chip revision %d\n", em->chip_revision);
		len += sprintf(page + len, "Memory mapped at addressrange 0x%0x->0x%0x%s\n", (unsigned int) em->mem, 
				(unsigned int) em->mem + (unsigned int) em->memsize, (em->mtrr_reg ? " (FIFOs using MTRR)" : ""));
		len += sprintf(page + len, "Displaybuffer resolution: %dx%d\n", em->dbuf_info.xsize, em->dbuf_info.ysize);
		len += sprintf(page + len, "Dicom set to %s\n", (em->dicom_tvout?"TV-out":"overlay"));
		if (em->dicom_tvout) {
			len += sprintf(page + len, "Using %s\n", (em->video_mode == EM8300_VIDEOMODE_PAL ? "PAL" : "NTSC"));
			len += sprintf(page + len, "Aspect is %s\n", (em->aspect_ratio == EM8300_ASPECTRATIO_4_3 ? "4:3" : "16:9"));
		} else {
			len += sprintf(page + len, "em9010 %s\n", (em->overlay_enabled ? "online" : "offline"));
			len += sprintf(page + len, "video mapped to screen coordinates %dx%d (%dx%d)\n", em->overlay_frame_xpos,
					em->overlay_frame_ypos, em->overlay_xres, em->overlay_yres);
		}
		len += sprintf(page + len, "%s audio output\n", (em->audio_mode == EM8300_AUDIOMODE_ANALOG ? "analog" : "digital"));
	}
	else {
		len += sprintf(page + len, "Microcode hasn't been loaded\n");
	}
	
        return len;
}
#endif

int init_em8300(struct em8300_s *em) {
	/* Setup parameters */
	static unsigned int *bt = use_bt865; 
    
	write_register(0x30000, read_register(0x30000));

	write_register(0x1f50, 0x123);

	if (read_register(0x1f50) == 0x123) {
		em->chip_revision = 2;
		if (0x40 & read_register(0x1c08)) {
			em->var_video_value = 3375; /* was 0xd34 = 3380 */
			em->mystery_divisor = 0x107ac;
			em->var_ucode_reg2 = 0x272;
			em->var_ucode_reg3 = 0x8272;
			if (0x20 & read_register(0x1c08)) {
				if(*bt) {
					em->var_ucode_reg1 = 0x800;
				} else {
					em->var_ucode_reg1 = 0x818;
				}
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

	pr_info("em8300_main.o: Chip revision: %d\n", em->chip_revision);
	pr_debug("em8300_main.o: use_bt865: %d\n", *bt);
	em8300_i2c_init(em);

	bt++;

	if (activate_loopback == 0) {
		em->clockgen_tvmode = CLOCKGEN_TVMODE_1;
		em->clockgen_overlaymode = CLOCKGEN_OVERLAYMODE_1;
	} else {
		em->clockgen_tvmode = CLOCKGEN_TVMODE_2;
		em->clockgen_overlaymode = CLOCKGEN_OVERLAYMODE_2;
	}

	pr_debug("em8300_main.o: activate_loopback: %d\n", activate_loopback);

	return 0;
}

void __exit em8300_exit(void)
{
	int card;
#ifdef CONFIG_DEVFS_FS
	int frame;
#endif
	char devname[64];
	
	for (card = 0; card < em8300_cards; card++) {
#ifdef CONFIG_DEVFS_FS
		for (frame = 3; frame >= 0; frame--) {
			devfs_unregister(em8300_handle[(card * 4) + frame]);
		}
#endif
#ifdef REGISTER_DSP
		unregister_sound_dsp(em8300[card].dsp_num);
#endif
	}
#ifdef CONFIG_PROC_FS
	sprintf(devname, "%s", EM8300_LOGNAME );
	if (em8300_proc != NULL) remove_proc_entry(devname, &proc_root);
#endif
#ifdef CONFIG_DEVFS_FS
	sprintf(devname, "%s", EM8300_LOGNAME);
	devfs_unregister_chrdev(em8300_major, devname);
	devfs_dealloc_major(DEVFS_SPECIAL_CHR, em8300_major);
#else
	unregister_chrdev(EM8300_MAJOR, EM8300_LOGNAME);
#endif
	release_em8300(em8300_cards);
}

int __init em8300_init(void)
{
	int card = 0;
	int frame = 3;
	struct em8300_s *em = NULL;

	char devname[32];
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc;
#endif
	memset(&em8300[0], 0, sizeof(em8300));
#ifdef REGISTER_DSP
	memset(&dsp_num_table, 0, sizeof(dsp_num_table));
#endif

#ifdef CONFIG_DEVFS_FS
	em8300_major = devfs_alloc_major(DEVFS_SPECIAL_CHR);
	sprintf(devname, "%s", EM8300_LOGNAME);
	if (devfs_register_chrdev(em8300_major, devname, &em8300_fops) < 0)
		goto err_chrdev;
#endif
#ifdef CONFIG_PROC_FS
	sprintf(devname, "%s", EM8300_LOGNAME);
	em8300_proc = create_proc_entry(devname, S_IFDIR | S_IRUGO | S_IXUGO, &proc_root);
	if (em8300_proc == NULL) {
		printk(KERN_ERR "em8300: unable to register proc entry!\n");
		goto err_chrdev;
	}
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,0)
	em8300_proc->owner = THIS_MODULE;
#endif
#endif
	/* Find EM8300 cards */
	em8300_cards = find_em8300();

	/* Initialize EM8300 cards */
	for (card = 0; card < em8300_cards; card++) {
		em = &em8300[card];
		
		em->irqmask = 0;
		
		em->encoder = NULL;
		em->eeprom = NULL;
		
		em->linecounter=0;
		
		init_em8300(em);

#ifdef CONFIG_PROC_FS
		sprintf(devname, "%s-%d", EM8300_LOGNAME, card );
		proc = create_proc_entry(devname, S_IFREG | S_IRUGO, em8300_proc);
		proc->data = (void *) em;
		proc->read_proc = em8300_proc_read;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,0)
		proc->owner = THIS_MODULE;
#endif
#endif
#ifdef CONFIG_DEVFS_FS
		sprintf(devname, "%s-%d", EM8300_LOGNAME, card );
		em8300_handle[(card * 4)] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, em8300_major, 
				(card * 4), S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
		sprintf(devname, "%s_mv-%d", EM8300_LOGNAME, card );
		em8300_handle[(card * 4) + 1] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, em8300_major, 
				(card * 4)+1, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
		sprintf(devname, "%s_ma-%d", EM8300_LOGNAME, card );
		em8300_handle[(card * 4) + 2] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, em8300_major,
				(card * 4) + 2, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
		sprintf(devname, "%s_sp-%d", EM8300_LOGNAME, card );
		em8300_handle[(card * 4) + 3] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, em8300_major,
				(card * 4) + 3, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
#endif
#ifdef REGISTER_DSP
		if ((em8300[card].dsp_num = register_sound_dsp(&em8300_dsp_audio_fops, -1)) < 0) {
			printk(KERN_ERR "em8300: cannot register oss audio device!\n");
			goto err_audio_dsp;
		}
		dsp_num_table[em8300[card].dsp_num >> 4 & 0x0f] = card + 1;
	        pr_debug("em8300: registered dsp %i for device %i\n", em8300[card].dsp_num >> 4 & 0x0f, card);
#endif
	}

#ifndef CONFIG_DEVFS_FS
	if (register_chrdev(EM8300_MAJOR, EM8300_LOGNAME, &em8300_fops)) {
		printk(KERN_ERR "em8300: unable to get major %d\n", EM8300_MAJOR);
		goto err_chrdev;
	}
#endif
	return 0;

#ifdef REGISTER_DSP
 err_audio_dsp:
#endif
 err_chrdev:
	while (card-- > 0) {
#ifdef CONFIG_DEVFS_FS
		while (frame-- > 0) {
			devfs_unregister(em8300_handle[(card * 4) + frame]);
		}
		frame = 3;
#endif
#ifdef REGISTER_DSP
		unregister_sound_dsp(em[card].dsp_num);
#endif
	}
#ifdef CONFIG_DEVFS_FS
	sprintf(devname, "%s", EM8300_LOGNAME);
	devfs_unregister_chrdev(em8300_major, devname);
	devfs_dealloc_major(DEVFS_SPECIAL_CHR, em8300_major);
#endif
	release_em8300(em8300_cards);
	return -ENODEV;
}

module_init(em8300_init);
module_exit(em8300_exit);
