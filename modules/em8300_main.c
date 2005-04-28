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
#include <linux/version.h>
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
#include <linux/poll.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <asm/segment.h>
#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include "em8300_compat24.h"
#include "encoder.h"

#include "em8300_reg.h"
#include <linux/em8300.h>
#include "em8300_fifo.h"
#include "em8300_registration.h"

#ifdef CONFIG_EM8300_IOCTL32
#include "em8300_ioctl32.h"
#endif

#if !defined(CONFIG_I2C_ALGOBIT) && !defined(CONFIG_I2C_ALGOBIT_MODULE)
#error "This needs the I2C Bit Banging Interface in your Kernel"
#endif

MODULE_AUTHOR("Henrik Johansson <henrikjo@post.utfors.se>");
MODULE_DESCRIPTION("EM8300 MPEG-2 decoder");
MODULE_SUPPORTED_DEVICE("em8300");
MODULE_LICENSE("GPL");
#if EM8300_MAJOR != 0
MODULE_ALIAS_CHARDEV_MAJOR(EM8300_MAJOR);
#endif

EXPORT_NO_SYMBOLS;

static unsigned int use_bt865[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
MODULE_PARM(use_bt865, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(use_bt865, "Set this to 1 if you have a bt865. It changes some internal register values. Defaults to 0.");

/*
 * Module params by Jonas Birmé (birme@jpl.nu)
 */
#ifdef CONFIG_EM8300_DICOMPAL
int dicom_other_pal[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
#else
int dicom_other_pal[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
#endif
MODULE_PARM(dicom_other_pal, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(dicom_other_pal, "If this is set, then some internal register values are swapped for PAL and NTSC. Defaults to 1.");

#ifdef CONFIG_EM8300_DICOMFIX
int dicom_fix[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
#else
int dicom_fix[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
#endif
MODULE_PARM(dicom_fix, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(dicom_fix, "If this is set then some internal register values are changed. Fixes green screen problems for some. Defaults to 1.");

#ifdef CONFIG_EM8300_DICOMCTRL
int dicom_control[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
#else
int dicom_control[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
#endif
MODULE_PARM(dicom_control, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(dicom_control, "If this is set then some internal register values are changed. Fixes green screen problems for some. Defaults to 1.");

#ifdef CONFIG_EM8300_UCODETIMEOUT
int bt865_ucode_timeout[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
#else
int bt865_ucode_timeout[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
#endif
MODULE_PARM(bt865_ucode_timeout, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(bt865_ucode_timeout, "Set this to 1 if you have a bt865 and get timeouts when uploading the microcode. Defaults to 0.");

#ifdef CONFIG_EM8300_LOOPBACK
int activate_loopback[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
#else
int activate_loopback[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
#endif
MODULE_PARM(activate_loopback, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(activate_loopback, "If you lose video after loading the modules or uploading the microcode set this to 1. Defaults to 0.");

int major = EM8300_MAJOR;
MODULE_PARM(major, "i");
MODULE_PARM_DESC(major, "Major number used for the devices. "
		 "0 means automatically assigned. "
		 "Defaults to " __MODULE_STRING(EM8300_MAJOR) ".");

static int em8300_cards,clients;

static struct em8300_s em8300[EM8300_MAX];
#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
static int dsp_num_table[16];
#endif

/* structure to keep track of the memory that has been allocated by
   the user via mmap() */
struct memory_info
{
	struct list_head item;
	long length;
	char *ptr;
};

static struct pci_device_id em8300_ids[] = {
	{ PCI_VENDOR_ID_SIGMADESIGNS, PCI_DEVICE_ID_SIGMADESIGNS_EM8300,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ 0 }
};

MODULE_DEVICE_TABLE(pci, em8300_ids);

static irqreturn_t em8300_irq(int irq, void *dev_id, struct pt_regs * regs)
{
	struct em8300_s *em = (struct em8300_s *) dev_id;
	int irqstatus;
	struct timeval tv;

	irqstatus = read_ucregister(Q_IrqStatus);

	if (irqstatus & 0x8000) {
		write_ucregister(Q_IrqMask, 0x0);
		writel(2, &em->mem[EM8300_INTERRUPT_ACK]);

		write_ucregister(Q_IrqStatus, 0x8000);

		if (irqstatus & IRQSTATUS_VIDEO_FIFO) {
			em8300_fifo_check(em->mvfifo);
		}

		if (irqstatus & IRQSTATUS_AUDIO_FIFO) {
			em8300_fifo_check(em->mafifo);
		}

		if (irqstatus & IRQSTATUS_VIDEO_VBL) {
			em8300_fifo_check(em->spfifo);
			em8300_video_check_ptsfifo(em);
			em8300_spu_check_ptsfifo(em);

			do_gettimeofday(&tv);
			em->irqtimediff = TIMEDIFF(tv, em->tv);
			em->tv = tv;
			em->irqcount++;
			wake_up(&em->vbi_wait);
		}

		write_ucregister(Q_IrqMask, em->irqmask);
		write_ucregister(Q_IrqStatus, 0x0000);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

static void release_em8300(struct em8300_s *em)
{
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
	writel(0, &em->mem[0x2000]);

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

static int em8300_io_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
	struct em8300_s *em = filp->private_data;
	int subdevice = EM8300_IMINOR(inode) % 4;

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
	int card = EM8300_IMINOR(inode) / 4;
	int subdevice = EM8300_IMINOR(inode) % 4;
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

	/* initalize the memory list */
	INIT_LIST_HEAD(&em->memory);

	switch (subdevice) {
	case EM8300_SUBDEVICE_CONTROL:
		em8300[card].nonblock[0] = ((filp->f_flags&O_NONBLOCK) == O_NONBLOCK);
		break;
	case EM8300_SUBDEVICE_AUDIO:
		em8300[card].nonblock[1] = ((filp->f_flags&O_NONBLOCK) == O_NONBLOCK);
		err = em8300_audio_open(em);
		break;
	case EM8300_SUBDEVICE_VIDEO:
		em8300[card].nonblock[2] = ((filp->f_flags&O_NONBLOCK) == O_NONBLOCK);
		if (!em->ucodeloaded) {
			return -ENODEV;
		}
		em8300_video_open(em);

		em8300_ioctl_enable_videoout(em, 1);

		em8300_video_setplaymode(em, EM8300_PLAYMODE_PLAY);
		break;
	case EM8300_SUBDEVICE_SUBPICTURE:
		em8300[card].nonblock[3] = ((filp->f_flags&O_NONBLOCK) == O_NONBLOCK);
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
	pr_debug("em8300_main.o: Opening device %d, Clients:%d\n", subdevice, clients);

	EM8300_MOD_INC_USE_COUNT;

	return(0);
}

static ssize_t em8300_io_write(struct file *file, const char * buf, size_t count, loff_t *ppos)
{
	struct em8300_s *em = file->private_data;
	int subdevice = EM8300_IMINOR(file->f_dentry->d_inode) % 4;

	switch (subdevice) {
	case EM8300_SUBDEVICE_VIDEO:
		em->nonblock[2] = ((file->f_flags&O_NONBLOCK) == O_NONBLOCK);
		return em8300_video_write(em, buf, count, ppos);
		break;
	case EM8300_SUBDEVICE_AUDIO:
		em->nonblock[1] = ((file->f_flags&O_NONBLOCK) == O_NONBLOCK);
		return em8300_audio_write(em, buf, count, ppos);
		break;
	case EM8300_SUBDEVICE_SUBPICTURE:
		em->nonblock[3] = ((file->f_flags&O_NONBLOCK) == O_NONBLOCK);
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
	int subdevice = EM8300_IMINOR(file->f_dentry->d_inode) % 4;

	if (subdevice != EM8300_SUBDEVICE_CONTROL) {
		return -EPERM;
	}

	switch (vma->vm_pgoff) {
	case 1: {
		/* fixme: we should count the total size of allocated memories
		   so we don't risk a out-of-memory or denial-of-service attack... */

		char *mem = 0;
		struct memory_info *info = NULL;
		unsigned long adr = 0;
		unsigned long size = vma->vm_end - vma->vm_start;
		unsigned long pages = (size+(PAGE_SIZE-1))/PAGE_SIZE;
		/* round up the memory */
		size = pages * PAGE_SIZE;

		/* allocate the physical contiguous memory */
		mem = (char*)kmalloc(pages*PAGE_SIZE, GFP_KERNEL);
		if( mem == NULL) {
			return -ENOMEM;
		}
		/* clear out the memory for sure */
		memset(mem, 0x00, pages*PAGE_SIZE);

		/* reserve all pages */
		for(adr = (long)mem; adr < (long)mem + size; adr += PAGE_SIZE) {
			SetPageReserved(virt_to_page(adr));
		}

		/* lock the area*/
		vma->vm_flags |=VM_LOCKED;

		/* remap the memory to user space */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,3)
		if (remap_page_range(vma->vm_start, virt_to_phys((void *)mem), size, vma->vm_page_prot)) {
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
		if (remap_page_range(vma, vma->vm_start, virt_to_phys((void *)mem), size, vma->vm_page_prot)) {
#else
		if (remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)mem) >> PAGE_SHIFT, size, vma->vm_page_prot)) {
#endif
			kfree(mem);
			return -EAGAIN;
		}

		/* put the physical address into the first dword of the memory */
		*((long*)mem) = virt_to_phys((void *)mem);

		/* keep track of the memory we have allocated */
		info = (struct memory_info*)vmalloc(sizeof(struct memory_info));
		if( NULL == info ) {
			kfree(mem);
			return -ENOMEM;
		}

		info->ptr = mem;
		info->length = size;
		list_add_tail(&info->item,&em->memory);

		break;
	}
	case 0:
		if (size > em->memsize) {
			return -EINVAL;
		}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,3)
		remap_page_range(vma->vm_start, em->adr, vma->vm_end - vma->vm_start, vma->vm_page_prot);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
		remap_page_range(vma, vma->vm_start, em->adr, vma->vm_end - vma->vm_start, vma->vm_page_prot);
#else
		remap_pfn_range(vma, vma->vm_start, em->adr >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot);
#endif
		vma->vm_file = file;
		atomic_inc(&file->f_dentry->d_inode->i_count);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static unsigned int em8300_poll(struct file *file, struct poll_table_struct *wait)
{
	struct em8300_s *em = file->private_data;
	int subdevice = EM8300_IMINOR(file->f_dentry->d_inode) % 4;
	unsigned int mask = 0;

	switch (subdevice) {
	case EM8300_SUBDEVICE_AUDIO:
		poll_wait(file, &em->mafifo->wait, wait);
		if (file->f_mode & FMODE_WRITE) {
			if (em8300_fifo_freeslots(em->mafifo)) {
				pr_debug("Poll audio - Free slots: %d\n", em8300_fifo_freeslots(em->mafifo));
				mask |= POLLOUT | POLLWRNORM;
			}
		}
		break;
	case EM8300_SUBDEVICE_VIDEO:
		poll_wait(file, &em->mvfifo->wait, wait);
		if (file->f_mode & FMODE_WRITE) {
			if (em8300_fifo_freeslots(em->mvfifo)) {
				pr_debug("Poll video - Free slots: %d\n", em8300_fifo_freeslots(em->mvfifo));
				mask |= POLLOUT | POLLWRNORM;
			}
		}
		break;
	case EM8300_SUBDEVICE_SUBPICTURE:
		poll_wait(file, &em->spfifo->wait, wait);
		if (file->f_mode & FMODE_WRITE) {
			if (em8300_fifo_freeslots(em->spfifo)) {
				pr_debug("Poll subpic - Free slots: %d\n", em8300_fifo_freeslots(em->spfifo));
				mask |= POLLOUT | POLLWRNORM;
			}
		}
	}

	return mask;
}

int em8300_io_release(struct inode* inode, struct file *filp)
{
	struct em8300_s *em = filp->private_data;
	int subdevice = EM8300_IMINOR(inode) % 4;

	switch (subdevice) {
	case EM8300_SUBDEVICE_AUDIO:
		em8300_audio_release(em);
		break;
	case EM8300_SUBDEVICE_VIDEO:
		em8300_video_release(em);
		em8300_ioctl_enable_videoout(em, 0);
		break;
	case EM8300_SUBDEVICE_SUBPICTURE:
		em8300_spu_release(em);    /* Do we need this one ? */
		break;
	}

	while( 0 == list_empty(&em->memory)) {
		unsigned long adr = 0;

		struct memory_info *info = list_entry(em->memory.next, struct memory_info, item);
		list_del(&info->item);

		for(adr = (long)info->ptr; adr < (long)info->ptr + info->length; adr += PAGE_SIZE) {
			ClearPageReserved(virt_to_page(adr));
		}

		kfree(info->ptr);
		vfree(info);
	}

	em->inuse[subdevice]--;

	clients--;
	pr_debug("em8300_main.o: Releasing device %d, clients:%d\n", subdevice, clients);

	EM8300_MOD_DEC_USE_COUNT;

	return(0);
}

struct file_operations em8300_fops = {
	owner: THIS_MODULE,
	write: em8300_io_write,
	ioctl: em8300_io_ioctl,
	mmap: em8300_io_mmap,
	poll: em8300_poll,
	open: em8300_io_open,
	release: em8300_io_release,
};

#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
static int em8300_dsp_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
	struct em8300_s *em = filp->private_data;
	return em8300_audio_ioctl(em, cmd, arg);
}

static int em8300_dsp_open(struct inode* inode, struct file* filp)
{
	int dsp_num = ((EM8300_IMINOR(inode) >> 4) & 0x0f);
	int card = dsp_num_table[dsp_num] - 1;
	int err = 0;

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
	pr_debug("em8300_main.o: Opening device %d, Clients:%d\n", EM8300_SUBDEVICE_AUDIO, clients);

	EM8300_MOD_INC_USE_COUNT;

	return(0);
}

static ssize_t em8300_dsp_write(struct file *file, const char * buf, size_t count, loff_t *ppos)
{
	struct em8300_s *em = file->private_data;
	return em8300_audio_write(em, buf, count, ppos);
}

static unsigned int em8300_dsp_poll(struct file *file, struct poll_table_struct *wait)
{
	struct em8300_s *em = file->private_data;
	unsigned int mask = 0;
	poll_wait(file, &em->mafifo->wait, wait);
	if (file->f_mode & FMODE_WRITE) {
		if (em8300_fifo_freeslots(em->mafifo)) {
			pr_debug("Poll dsp - Free slots: %d\n", em8300_fifo_freeslots(em->mafifo));
			mask |= POLLOUT | POLLWRNORM;
		}
	}
	return mask;
}

int em8300_dsp_release(struct inode* inode, struct file* filp)
{
	struct em8300_s *em = filp->private_data;

	em8300_audio_release(em);

	em->inuse[EM8300_SUBDEVICE_AUDIO]--;

	clients--;
	pr_debug("em8300_main.o: Releasing device %d, clients:%d\n", EM8300_SUBDEVICE_AUDIO, clients);

	EM8300_MOD_DEC_USE_COUNT;

	return(0);
}

static struct file_operations em8300_dsp_audio_fops = {
	owner: THIS_MODULE,
	write: em8300_dsp_write,
	ioctl: em8300_dsp_ioctl,
	poll: em8300_dsp_poll,
	open: em8300_dsp_open,
	release: em8300_dsp_release,
};
#endif

static int init_em8300(struct em8300_s *em)
{
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
				if (use_bt865[em->card_nr]) {
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
	pr_debug("em8300_main.o: use_bt865: %d\n", use_bt865[em->card_nr]);
	em8300_i2c_init(em);

	if (activate_loopback[em->card_nr] == 0) {
		em->clockgen_tvmode = CLOCKGEN_TVMODE_1;
		em->clockgen_overlaymode = CLOCKGEN_OVERLAYMODE_1;
	} else {
		em->clockgen_tvmode = CLOCKGEN_TVMODE_2;
		em->clockgen_overlaymode = CLOCKGEN_OVERLAYMODE_2;
	}

	pr_debug("em8300_main.o: activate_loopback: %d\n", activate_loopback);

	return 0;
}

static int __devinit em8300_probe(struct pci_dev *dev,
				  const struct pci_device_id *pci_id)
{
	unsigned char revision;
	struct em8300_s *em;
	int result;

	em = &em8300[em8300_cards];
	em->dev = dev;
	em->card_nr = em8300_cards;
	em->adr = dev->resource[0].start;
	em->memsize = 1024 * 1024;

	pci_enable_device(dev);
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

	init_waitqueue_head(&em->video_ptsfifo_wait);
	init_waitqueue_head(&em->vbi_wait);
	init_waitqueue_head(&em->sp_ptsfifo_wait);

	result = request_irq(dev->irq, em8300_irq, SA_SHIRQ | SA_INTERRUPT, "em8300", (void *) em);

	if (result == -EINVAL) {
		printk(KERN_ERR "em8300: Bad irq number or handler\n");
		return -EINVAL;
	}

	pci_set_master(dev);
	pci_set_drvdata(dev, em);

	em->irqmask = 0;
	em->encoder = NULL;
	em->linecounter=0;

	init_em8300(em);

	em8300_register_card(em);

#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
	if ((em->dsp_num = register_sound_dsp(&em8300_dsp_audio_fops, -1)) < 0) {
		printk(KERN_ERR "em8300: cannot register oss audio device!\n");
	} else {
		dsp_num_table[em->dsp_num >> 4 & 0x0f] = em8300_cards + 1;
		pr_debug("em8300: registered dsp %i for device %i\n", em->dsp_num >> 4 & 0x0f, em8300_cards);
	}
#endif

	em8300_cards++;
	return 0;
}

static void __devexit em8300_remove(struct pci_dev *pci)
{
	struct em8300_s *em = pci_get_drvdata(pci);

	if (em) {
		if (em->ucodeloaded == 1)
			em8300_disable_card(em);

#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
		unregister_sound_dsp(em->dsp_num);
#endif
		em8300_unregister_card(em);

		release_em8300(em);
	}

	pci_set_drvdata(pci, NULL);
	pci_disable_device(pci);
}

static struct pci_driver em8300_driver = {
	.name     = "Sigma Designs EM8300",
	.id_table = em8300_ids,
	.probe    = em8300_probe,
	.remove   = __devexit_p(em8300_remove),
};

static void __exit em8300_exit(void)
{
#ifdef CONFIG_EM8300_IOCTL32
	em8300_ioctl32_exit();
#endif

	pci_unregister_driver(&em8300_driver);

	unregister_chrdev(major, EM8300_LOGNAME);

	em8300_unregister_driver();
}

static int __init em8300_init(void)
{
	int err;

	//memset(&em8300, 0, sizeof(em8300) * EM8300_MAX);
#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
	memset(&dsp_num_table, 0, sizeof(dsp_num_table));
#endif

	em8300_register_driver();

	if (major) {
		if (register_chrdev(major, EM8300_LOGNAME, &em8300_fops)) {
			printk(KERN_ERR "em8300: unable to get major %d\n", major);
			err = -ENODEV;
			goto err_chrdev;
		}
	}
	else {
		int m = register_chrdev(major, EM8300_LOGNAME, &em8300_fops);
		if (m > 0) {
			major = m;
		}
		else {
			printk(KERN_ERR "em8300: unable to get any majo\n");
			err = -ENODEV;
			goto err_chrdev;
		}
	}

	if ((err = pci_module_init(&em8300_driver)) < 0) {
#ifdef MODULE
		printk(KERN_ERR "Sigmadesigns EM8300 not found or device busy\n");
#endif
		goto err_init;
	}

#ifdef CONFIG_EM8300_IOCTL32
	em8300_ioctl32_init();
#endif

	return 0;

 err_init:
	unregister_chrdev(major, EM8300_LOGNAME);

 err_chrdev:
	em8300_unregister_driver();
	return err;
}

module_init(em8300_init);
module_exit(em8300_exit);
