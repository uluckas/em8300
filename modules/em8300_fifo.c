/*
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

#define __NO_VERSION__

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

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include "em8300_reg.h"
#include <linux/em8300.h>
#include "em8300_fifo.h"



int em8300_fifo_init(struct em8300_s *em, struct fifo_s *f, int start, int wrptr, int rdptr, int pcisize, int slotsize, int fifotype)
{
	int i;
	dma_addr_t phys;

	f->em = em;
	f->preprocess_ratio = 1;
	f->preprocess_cb = NULL;
	f->preprocess_buffer = NULL;

	f->type = fifotype;

	f->writeptr = (unsigned * volatile) ucregister_ptr(wrptr);
	f->readptr = (unsigned * volatile) ucregister_ptr(rdptr);

	switch (f->type) {
	case FIFOTYPE_AUDIO:
		f->slotptrsize = 3;
		f->slots.a = (struct audio_fifoslot_s *) ucregister_ptr(start);
		f->nslots = read_ucregister(pcisize) / 3;
		f->preprocess_buffer=kmalloc(slotsize, GFP_KERNEL);
		if (!f->preprocess_buffer)
			return -ENOMEM;
		break;
	case FIFOTYPE_VIDEO:
		f->slotptrsize = 4;
		f->slots.v = (struct video_fifoslot_s *) ucregister_ptr(start);
		f->nslots = read_ucregister(pcisize) / 4;
		break;
	}

	f->slotsize = slotsize;
	f->start = ucregister(start) - 0x1000;
	f->threshold = f->nslots / 2;

	f->bytes = 0;

	if (f->fifobuffer) {
		kfree(f->fifobuffer);
	}

	f->fifobuffer = pci_alloc_consistent(f->em->dev, f->nslots * f->slotsize, &f->phys_base);
	if (f->fifobuffer == NULL) {
		return -ENOMEM;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
	init_waitqueue(&f->wait);
#else
	init_waitqueue_head(&f->wait);
#endif

	for (i = 0; i < f->nslots; i++) {
		phys = f->phys_base + i * f->slotsize;
		switch (f->type) {
		case FIFOTYPE_AUDIO:
			writel(phys >> 16, &f->slots.a[i].physaddress_hi);
			writel(phys & 0xffff, &f->slots.a[i].physaddress_lo);
			writel(f->slotsize, &f->slots.a[i].slotsize);
			break;
		case FIFOTYPE_VIDEO:
			writel(0, &f->slots.v[i].flags);
			writel(phys >> 16, &f->slots.v[i].physaddress_hi);
			writel(phys & 0xffff, &f->slots.v[i].physaddress_lo);
			writel(f->slotsize, &f->slots.v[i].slotsize);
		}
	}

	spin_lock_init(&f->lock);
	f->valid = 1;

	return 0;
}

void em8300_fifo_free(struct fifo_s *f)
{
	if (f) {
		if(f->valid && f->fifobuffer) {
			pci_free_consistent(f->em->dev, f->nslots * f->slotsize, f->fifobuffer, f->phys_base);
		}
		if(f->valid && f->preprocess_buffer) {
			kfree(f->preprocess_buffer);
		}
		kfree(f);
	}
}

struct fifo_s *em8300_fifo_alloc()
{
	struct fifo_s *f = kmalloc(sizeof(struct fifo_s), GFP_KERNEL);
	if (f) {
		memset(f, 0, sizeof(struct fifo_s));
	}
	return f;
}

int em8300_fifo_check(struct fifo_s *fifo)
{
	int freeslots;

	if (!fifo || !fifo->valid) {
		return -1;
	}

	freeslots = em8300_fifo_freeslots(fifo);

	if (freeslots > fifo->threshold) {
		wake_up_interruptible(&fifo->wait);
	}

	return 0;
}

int em8300_fifo_sync(struct fifo_s *fifo)
{
	unsigned int safe_jiff = jiffies;

	while (readl(fifo->writeptr) != readl(fifo->readptr)) {
		interruptible_sleep_on_timeout(&fifo->wait, 3 * HZ);
		if (time_after_eq(jiffies, safe_jiff + (3 * HZ))) {
			printk(KERN_ERR "em8300.o: FIFO sync timeout during sync\n");
			return -EINTR;
		}

		if (signal_pending(current)) {
			printk(KERN_ERR "em8300.o: FIFO sync interrupted\n");
			return -EINTR;
		}
	}

	return 0;
}

int em8300_fifo_write_nolock(struct fifo_s *fifo, int n, const char *userbuffer, int flags)
{
	int freeslots, writeindex, i, bytes_transferred = 0, copysize;

	if (!fifo || !fifo->valid) {
		return -1;
	}

	freeslots = em8300_fifo_freeslots(fifo);
	writeindex = ((int)readl(fifo->writeptr) - fifo->start) / fifo->slotptrsize;
	for (i = 0; i < freeslots && n; i++) {
		copysize = n < fifo->slotsize / fifo->preprocess_ratio ? n : fifo->slotsize / fifo->preprocess_ratio;

		switch (fifo->type) {
		case FIFOTYPE_AUDIO:
			writel(copysize * fifo->preprocess_ratio, &fifo->slots.a[writeindex].slotsize);
			break;
		case FIFOTYPE_VIDEO:
			writel(flags, &fifo->slots.v[writeindex].flags);
			writel(copysize * fifo->preprocess_ratio, &fifo->slots.v[writeindex].slotsize);
			break;
		}

		if (fifo->preprocess_cb) {
			fifo->preprocess_cb(fifo->em, fifo->fifobuffer + writeindex * fifo->slotsize, userbuffer, copysize);
		} else {
			copy_from_user(fifo->fifobuffer + writeindex * fifo->slotsize, userbuffer, copysize);
		}

		writeindex++;
		writeindex %= fifo->nslots;
		n -= copysize;
		userbuffer += copysize;
		bytes_transferred += copysize;
		fifo->bytes += copysize;
	}
	writel(fifo->start + writeindex * fifo->slotptrsize, fifo->writeptr);

	return bytes_transferred;
}

int em8300_fifo_write(struct fifo_s *fifo, int n, const char *userbuffer, int flags)
{
	int ret;

	spin_lock(&fifo->lock);
	ret = em8300_fifo_write_nolock(fifo, n, userbuffer, flags);

	spin_unlock(&fifo->lock);
	return ret;
}

int em8300_fifo_writeblocking_nolock(struct fifo_s *fifo, int n, const char *userbuffer, int flags)
{
	int total_bytes_written = 0, copy_size;

	if (!fifo->valid) {
		return -EPERM;
	}


	while (n) {
		copy_size = em8300_fifo_write_nolock(fifo, n, userbuffer, flags);

		if (copy_size < 0) {
			return -EIO;
		}

		n -= copy_size;
		userbuffer += copy_size;
		total_bytes_written += copy_size;

		if (!copy_size) {
			struct em8300_s *em = fifo->em;
			int running = 1;

			//printk("Fifo Full %p\n", fifo);

			running = (running && (read_ucregister(MV_SCRSpeed) > 0));
			running = (running && (em->video_playmode == EM8300_PLAYMODE_PLAY));
			/* FIXME: are these all conditions for a running DMA engine? */

			if (running) {
				interruptible_sleep_on_timeout(&fifo->wait, HZ);
				if (!em8300_fifo_freeslots(fifo)) {
					printk("Fifo still full, trying stop %p\n", fifo);
					em8300_video_setplaymode(em, EM8300_PLAYMODE_STOPPED);
					em8300_video_setplaymode(em, EM8300_PLAYMODE_PLAY);

					interruptible_sleep_on_timeout(&fifo->wait, 3 * HZ);
					if (!em8300_fifo_freeslots(fifo)) {
						printk(KERN_ERR "em8300.o: FIFO sync timeout during blocking write, n = %d, copy_size = %d, total = %d, free slots = %d\n", n, copy_size, total_bytes_written, em8300_fifo_freeslots(fifo));
						if (total_bytes_written) {
							return total_bytes_written;
						} else {
							return -EINTR;
						}
					}
				}
			} else {
				interruptible_sleep_on(&fifo->wait);
			}
		}

		if (signal_pending(current)) {
			if (total_bytes_written) {
				return total_bytes_written;
			} else {
				return -EINTR;
			}
		}

	}

	// printk(KERN_ERR "em8300.o: count = %d\n", total_bytes_written);
	// printk(KERN_ERR "em8300.o: time  = %d\n", jiffies - safe_jiff);
	return total_bytes_written;
}

int em8300_fifo_writeblocking(struct fifo_s *fifo, int n, const char *userbuffer, int flags)
{
	int ret;

	spin_lock(&fifo->lock);
	ret = em8300_fifo_writeblocking_nolock(fifo, n, userbuffer, flags);

	spin_unlock(&fifo->lock);
	return ret;
}

int em8300_fifo_freeslots(struct fifo_s *fifo)
{
	return (((int)readl(fifo->readptr) - (int)readl(fifo->writeptr)) / fifo->slotptrsize + fifo->nslots - 1) % fifo->nslots;
}

void em8300_fifo_statusmsg(struct fifo_s *fifo, char *str)
{
	int freeslots = em8300_fifo_freeslots(fifo);
	sprintf(str,"Free slots: %d/%d", freeslots, fifo->nslots);
}

int em8300_fifo_calcbuffered(struct fifo_s *fifo)
{
	int readindex, writeindex, i, n;

	writeindex = ((int)readl(fifo->writeptr) - fifo->start) / fifo->slotptrsize;
	readindex = ((int)readl(fifo->readptr) - fifo->start) / fifo->slotptrsize;
	n = 0;
	i = readindex;
	while (i != writeindex) {
		switch (fifo->type) {
		case FIFOTYPE_AUDIO:
			n += readl(&fifo->slots.a[i].slotsize);
			break;
		case FIFOTYPE_VIDEO:
			n += readl(&fifo->slots.v[i].slotsize);
			break;
		}
		i++;
		i &= fifo->nslots-1;
	}

	return n;
}

int em8300_fifo_isempty(struct fifo_s *fifo)
{
	return (readl(fifo->writeptr) == readl(fifo->readptr));
}
