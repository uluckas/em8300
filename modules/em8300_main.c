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
#include <linux/autoconf.h>
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
#include <linux/wrapper.h>	/* for mem_map_reserve */
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,48)
#include <linux/interrupt.h>
#endif
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include "encoder.h"

#include "em8300_reg.h"
#include <linux/em8300.h>
#include "em8300_fifo.h"

#ifdef CONFIG_EM8300_IOCTL32
#include "em8300_ioctl32.h"
#endif

/* It seems devfs will implement a new scheme of enumerating minor numbers.
 * Currently it seems broken. But that is why we added these macros.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#define EM8300_MINOR(inode) (MINOR((inode)->i_rdev) % 4)
#define EM8300_CARD(inode) (MINOR((inode)->i_rdev) / 4)
#else
#define EM8300_MINOR(inode) (minor(inode->i_rdev) % 4)
#define EM8300_CARD(inode) (minor(inode->i_rdev) / 4)
#endif

#if !defined(CONFIG_I2C_ALGOBIT) && !defined(CONFIG_I2C_ALGOBIT_MODULE)
#error "This needs the I2C Bit Banging Interface in your Kernel"
#endif

MODULE_AUTHOR("Henrik Johansson <henrikjo@post.utfors.se>");
MODULE_DESCRIPTION("EM8300 MPEG-2 decoder");
MODULE_SUPPORTED_DEVICE("em8300");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,9)
MODULE_LICENSE("GPL");
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,18)
EXPORT_NO_SYMBOLS;
#endif

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

static int em8300_cards,clients;

static struct em8300_s em8300[EM8300_MAX];
#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
static int dsp_num_table[16];
#endif
#ifdef CONFIG_DEVFS_FS
devfs_handle_t em8300_handle[EM8300_MAX*4];
#endif
#ifdef CONFIG_PROC_FS
struct proc_dir_entry *em8300_proc;
#endif

/* structure to keep track of the memory that has been allocated by
   the user via mmap() */
struct memory_info
{
	struct list_head item;
	long length;
	char *ptr;
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,69)
static irqreturn_t em8300_irq(int irq, void *dev_id, struct pt_regs * regs)
#else
static void em8300_irq(int irq, void *dev_id, struct pt_regs * regs)
#endif
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,69)
		return IRQ_HANDLED;
#endif
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,69)
	return IRQ_NONE;
#endif
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
}

static int find_em8300(void)
{
	struct pci_dev *dev = NULL;
	unsigned char revision;
	struct em8300_s *em;
	unsigned int em8300_n = 0;
	int result;

	while ((dev = pci_find_device(PCI_VENDOR_ID_SIGMADESIGNS, PCI_DEVICE_ID_SIGMADESIGNS_EM8300, dev))) {
		em = &em8300[em8300_n];
		em->dev = dev;
		em->card_nr = em8300_n;
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
	int subdevice = EM8300_MINOR(inode);

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
	int card = EM8300_CARD(inode);
	int subdevice = EM8300_MINOR(inode);
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,48)
	MOD_INC_USE_COUNT;
#endif

	return(0);
}

static ssize_t em8300_io_write(struct file *file, const char * buf, size_t count, loff_t *ppos)
{
	struct em8300_s *em = file->private_data;
	int subdevice = EM8300_MINOR(file->f_dentry->d_inode);

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
	int subdevice = EM8300_MINOR(file->f_dentry->d_inode);

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
			mem_map_reserve(virt_to_page(adr));
		}

		/* lock the area*/
		vma->vm_flags |=VM_LOCKED;

		/* remap the memory to user space */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,3)
		if (remap_page_range(vma->vm_start, virt_to_phys((void *)mem), size, vma->vm_page_prot)) {
#else
		if (remap_page_range(vma, vma->vm_start, virt_to_phys((void *)mem), size, vma->vm_page_prot)) {
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
#else
		remap_page_range(vma, vma->vm_start, em->adr, vma->vm_end - vma->vm_start, vma->vm_page_prot);
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
	int subdevice = EM8300_MINOR(file->f_dentry->d_inode);
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
	int subdevice = EM8300_MINOR(inode);

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

	while( 0 == list_empty(&em->memory)) {
		unsigned long adr = 0;

		struct memory_info *info = list_entry(em->memory.next, struct memory_info, item);
		list_del(&info->item);

		for(adr = (long)info->ptr; adr < (long)info->ptr + info->length; adr += PAGE_SIZE) {
			mem_map_unreserve(virt_to_page(adr));
		}

		kfree(info->ptr);
		vfree(info);
	}

	em->inuse[subdevice]--;

	clients--;
	pr_debug("em8300_main.o: Releasing device %d, clients:%d\n", subdevice, clients);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,48)
	MOD_DEC_USE_COUNT;
#endif

	return(0);
}

static struct file_operations em8300_fops = {
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	int dsp_num = ((MINOR(inode->i_rdev) >> 4) & 0x0f);
#else
	int dsp_num = ((minor(inode->i_rdev) >> 4) & 0x0f);
#endif
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,48)
	MOD_INC_USE_COUNT;
#endif

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,48)
	MOD_DEC_USE_COUNT;
#endif

	return(0);
}

static struct file_operations em8300_dsp_audio_fops = {
	write: em8300_dsp_write,
	ioctl: em8300_dsp_ioctl,
	poll: em8300_dsp_poll,
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
	len += sprintf(page + len, "em8300 module version %s\n", EM8300_VERSION);
	if (em->ucodeloaded) {
		/* Device information */
		len += sprintf(page + len, "Card revision %d\n", em->pci_revision);
		len += sprintf(page + len, "Chip revision %d\n", em->chip_revision);
		len += sprintf(page + len, "Video encoder: %s at address 0x%02x on %s\n", (em->encoder_type == ENCODER_BT865) ? "BT865" : (em->encoder_type == ENCODER_ADV7170) ? "ADV7170" : (em->encoder_type == ENCODER_ADV7175) ? "ADV7175" : "unknown", em->encoder->addr, em->encoder->adapter->name);
		len += sprintf(page + len, "Memory mapped at addressrange 0x%0lx->0x%0lx%s\n", (unsigned long int) em->mem,
				(unsigned long int) em->mem + (unsigned long int) em->memsize, (em->mtrr_reg ? " (FIFOs using MTRR)" : ""));
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

int init_em8300(struct em8300_s *em)
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

void __exit em8300_exit(void)
{
	int card;
#ifdef CONFIG_DEVFS_FS
	int frame;
#endif
	char devname[64];

#ifdef CONFIG_EM8300_IOCTL32
	em8300_ioctl32_exit();
#endif

	for (card = 0; card < em8300_cards; card++) {
#ifdef CONFIG_DEVFS_FS
		for (frame = 3; frame >= 0; frame--) {
			devfs_unregister(em8300_handle[(card * 4) + frame]);
		}
#endif
#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
		unregister_sound_dsp(em8300[card].dsp_num);
#endif
	}
#ifdef CONFIG_PROC_FS
	sprintf(devname, "%s", EM8300_LOGNAME );
	if (em8300_proc != NULL) remove_proc_entry(devname, &proc_root);
#endif
#if defined(CONFIG_DEVFS_FS) && LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	sprintf(devname, "%s", EM8300_LOGNAME);
	devfs_unregister_chrdev(EM8300_MAJOR, devname);
#endif
	unregister_chrdev(EM8300_MAJOR, EM8300_LOGNAME);
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
	//memset(&em8300, 0, sizeof(em8300) * EM8300_MAX);
#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
	memset(&dsp_num_table, 0, sizeof(dsp_num_table));
#endif

#if defined(CONFIG_DEVFS_FS) && LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	sprintf(devname, "%s", EM8300_LOGNAME);
	devfs_register_chrdev(EM8300_MAJOR, devname, &em8300_fops);
#endif
#ifdef CONFIG_PROC_FS
	sprintf(devname, "%s", EM8300_LOGNAME);
	em8300_proc = create_proc_entry(devname, S_IFDIR | S_IRUGO | S_IXUGO, &proc_root);
	if (em8300_proc == NULL) {
		printk(KERN_ERR "em8300: unable to register proc entry!\n");
		goto err_chrdev;
	}
	em8300_proc->owner = THIS_MODULE;
#endif
#ifdef CONFIG_SOUND_MODULE
	//request_module("soundcore");
#endif

	/* Find EM8300 cards */
	em8300_cards = find_em8300();

	/* Initialize EM8300 cards */
	for (card = 0; card < em8300_cards; card++) {
		em = &em8300[card];

		em->irqmask = 0;

		em->encoder = NULL;

		em->linecounter=0;

		init_em8300(em);

#ifdef CONFIG_PROC_FS
		sprintf(devname, "%d", card );
		proc = create_proc_entry(devname, S_IFREG | S_IRUGO, em8300_proc);
		proc->data = (void *) em;
		proc->read_proc = em8300_proc_read;
		proc->owner = THIS_MODULE;
#endif
#ifdef CONFIG_DEVFS_FS
		sprintf(devname, "%s-%d", EM8300_LOGNAME, card );
		em8300_handle[(card * 4)] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, EM8300_MAJOR,
				(card * 4), S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
		sprintf(devname, "%s_mv-%d", EM8300_LOGNAME, card );
		em8300_handle[(card * 4) + 1] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, EM8300_MAJOR,
				(card * 4)+1, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
		sprintf(devname, "%s_ma-%d", EM8300_LOGNAME, card );
		em8300_handle[(card * 4) + 2] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, EM8300_MAJOR,
				(card * 4) + 2, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
		sprintf(devname, "%s_sp-%d", EM8300_LOGNAME, card );
		em8300_handle[(card * 4) + 3] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, EM8300_MAJOR,
				(card * 4) + 3, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
#endif
#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
		if ((em8300[card].dsp_num = register_sound_dsp(&em8300_dsp_audio_fops, -1)) < 0) {
			printk(KERN_ERR "em8300: cannot register oss audio device!\n");
			goto err_audio_dsp;
		}
		dsp_num_table[em8300[card].dsp_num >> 4 & 0x0f] = card + 1;
		pr_debug("em8300: registered dsp %i for device %i\n", em8300[card].dsp_num >> 4 & 0x0f, card);
#endif
	}

	if (register_chrdev(EM8300_MAJOR, EM8300_LOGNAME, &em8300_fops)) {
		printk(KERN_ERR "em8300: unable to get major %d\n", EM8300_MAJOR);
		goto err_chrdev;
	}

#ifdef CONFIG_EM8300_IOCTL32
	em8300_ioctl32_init();
#endif

	return 0;

#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
 err_audio_dsp:
#endif
 err_chrdev:
	while (card-- > 0) {
		while (frame-- > 0) {
#ifdef CONFIG_DEVFS_FS
			devfs_unregister(em8300_handle[(card * 4) + frame]);
#endif
		}
		frame = 3;
#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
		unregister_sound_dsp(em[card].dsp_num);
#endif
	}
#if defined(CONFIG_DEVFS_FS) && LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	sprintf(devname, "%s", EM8300_LOGNAME);
	devfs_unregister_chrdev(EM8300_MAJOR, devname);
#endif
	release_em8300(em8300_cards);
	return -ENODEV;
}

module_init(em8300_init);
module_exit(em8300_exit);
