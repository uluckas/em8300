#define __NO_VERSION__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include "em8300_reg.h"
#include <linux/em8300.h>

#include <linux/soundcard.h>

#include "em8300_reg.c"

static int upload_block(struct em8300_s *em, int blocktype, int offset, int len, unsigned char *buf)
{
	int i, val;

	switch (blocktype) {
	case 4:
		offset *= 2;
		writel(offset & 0xffff, &em->mem[0x1c11]);
		writel((offset >> 16) & 0xffff, &em->mem[0x1c12]);
		writel(len, &em->mem[0x1c13]);
		writel(len, &em->mem[0x1c14]);
		writel(0, &em->mem[0x1c15]);
		writel(1, &em->mem[0x1c16]);
		writel(1, &em->mem[0x1c17]);
		writel(offset & 0xffff, &em->mem[0x1c18]);
		writel((offset >> 16) & 0xffff, &em->mem[0x1c19]);

		writel(1, &em->mem[0x1c1a]);

		for (i = 0; i < len; i += 4) {
			val = (buf[i + 2] << 24) | (buf[i + 3] << 16) | (buf[i] << 8) | buf[i + 1];
			writel(val, &em->mem[0x11800]);
		}

		if (em8300_waitfor(em, 0x1c1a, 0, 1)) {
			return -ETIME;
		}
		break;
	case 1:
		for (i = 0; i < len; i += 4) {
			val = (buf[i + 1] << 24) | (buf[i] << 16) | (buf[i + 3] << 8) | buf[i + 2];
			writel(val, &em->mem[offset / 2 + i / 4]);
		}
		break;
	case 2:
		for (i = 0; i < len; i += 2) {
			val = (buf[i + 1] << 8) | buf[i];
			writel(val, &em->mem[0x1000 + offset + i / 2]);
		}
		break;
	}

	return 0;
}

static
int upload_prepare(struct em8300_s *em)
{
	writel(0x1ff00, &em->mem[0x30000]);
	writel(0x123, &em->mem[0x1f50]);

	writel(0x0, &em->mem[0x20001]);
	writel(0x2, &em->mem[0x2000]);
	writel(0x0, &em->mem[0x2000]);
	writel(0xffff, &em->mem[0x1ff8]);
	writel(0xffff, &em->mem[0x1ff9]);
	writel(0xff00, &em->mem[0x1ff8]);
	writel(0xff00, &em->mem[0x1ff9]);

	if (em->chip_revision == 1) {
		writel(0x8c7, &em->mem[0x1c04]);
		writel(0x80, &em->mem[0x1c00]);
		writel(0xc7, &em->mem[0x1c04]);
	}
	writel(em->var_ucode_reg3, &em->mem[0x1c04]);
	writel(em->var_ucode_reg1, &em->mem[0x1c00]);
	writel(em->var_ucode_reg2, &em->mem[0x1c04]);

	/* em->mem[0x1c08]; */
	writel(0x8, &em->mem[0x1c10]);
	writel(0x8, &em->mem[0x1c20]);
	writel(0x8, &em->mem[0x1c30]);
	writel(0x8, &em->mem[0x1c40]);
	writel(0x8, &em->mem[0x1c50]);
	writel(0x8, &em->mem[0x1c60]);
	writel(0x8, &em->mem[0x1c70]);
	writel(0x8, &em->mem[0x1c80]);
	writel(0x10, &em->mem[0x1c90]);
	writel(0x10, &em->mem[0x1ca0]);
	writel(0x8, &em->mem[0x1cb0]);
	writel(0x8, &em->mem[0x1cc0]);
	writel(0x8, &em->mem[0x1cd0]);
	writel(0x8, &em->mem[0x1ce0]);
	writel(0x5555, &em->mem[0x1c01]);
	writel(0x55a, &em->mem[0x1c02]);
	writel(0x0, &em->mem[0x1c03]);

	return 0;
}

void em8300_ucode_upload(struct em8300_s *em, void *ucode, int ucode_size)
{
	int flags, offset, len;
	unsigned char *p;
	int memcount, i;
	char regname[128];

	upload_prepare(em);

	memcount = 0;

	p = ucode;
	while (memcount < ucode_size) {
		flags =  p[0] | (p[1] << 8); p += 2;
		offset = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); p += 4;
		len = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); p += 4;
		memcount += 10;
		len *= 2;

		if (!flags)
			break;

		switch (flags & 0xf00) {
		case 0:
			upload_block(em, flags, offset, len, p);
			break;
		case 0x200:
			for (i = 0;i < len; i++) {
				if (p[i]) {
					regname[i] = p[i] ^ 0xff;
				} else {
					break;
				}
			}
			regname[i] = 0;

			for (i = 0; i < MAX_UCODE_REGISTER; i++) {
				if (!strcmp(ucodereg_names[i], regname)) {
					em->ucode_regs[i] = 0x1000 + offset;
					break;
				}
			}
			break;
		}
		memcount += len;
		p += len;
	}
}

#if defined(CONFIG_FW_LOADER) || defined(CONFIG_FW_LOADER_MODULE)

#include <linux/firmware.h>
#include "em8300_fifo.h"
#include "em8300_registration.h"

typedef enum {
	AUDIO_DRIVER_NONE,
	AUDIO_DRIVER_OSSLIKE,
	AUDIO_DRIVER_OSS,
	AUDIO_DRIVER_ALSA,
	AUDIO_DRIVER_MAX
} audio_driver_t;

extern audio_driver_t audio_driver_nr[EM8300_MAX];

void em8300_require_ucode(struct em8300_s *em)
{
	if (!em->ucodeloaded) {
		const struct firmware *fw_entry = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		if (request_firmware(&fw_entry, "em8300.bin", &em->dev->dev) != 0) {
			printk(KERN_ALERT "%s: firmware %s is missing, cannot start.\n",
			       em->dev->dev.bus_id, "em8300.bin");
			return;
		}
#else
		if (request_firmware(&fw_entry, "em8300.bin", em->dev->slot_name) != 0) {
			printk(KERN_ALERT "%s: firmware %s is missing, cannot start.\n",
			       em->dev->slot_name, "em8300.bin");
			return;
		}
#endif
		em8300_ucode_upload(em, fw_entry->data, fw_entry->size);

		em8300_dicom_init(em);

		if (em8300_video_setup(em)) {
			return;
		}

		if (em->mvfifo) {
			em8300_fifo_free(em->mvfifo);
		}
		if ((audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSSLIKE)
		    || (audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSS))
			if (em->mafifo) {
				em8300_fifo_free(em->mafifo);
			}
		if (em->spfifo) {
			em8300_fifo_free(em->spfifo);
		}

		if (!(em->mvfifo = em8300_fifo_alloc())) {
			return;
		}

		if ((audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSSLIKE)
		    || (audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSS))
			if (!(em->mafifo = em8300_fifo_alloc())) {
				return;
			}

		if (!(em->spfifo = em8300_fifo_alloc())) {
			return;
		}

		em8300_fifo_init(em,em->mvfifo, MV_PCIStart, MV_PCIWrPtr, MV_PCIRdPtr, MV_PCISize, 0x900, FIFOTYPE_VIDEO);
		if ((audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSSLIKE)
		    || (audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSS))
			em8300_fifo_init(em,em->mafifo, MA_PCIStart, MA_PCIWrPtr, MA_PCIRdPtr, MA_PCISize, 0x1000, FIFOTYPE_AUDIO);
		//	em8300_fifo_init(em,em->spfifo, SP_PCIStart, SP_PCIWrPtr, SP_PCIRdPtr, SP_PCISize, 0x1000, FIFOTYPE_VIDEO);
		em8300_fifo_init(em,em->spfifo, SP_PCIStart, SP_PCIWrPtr, SP_PCIRdPtr, SP_PCISize, 0x800, FIFOTYPE_VIDEO);
		em8300_spu_init(em);

		if ((audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSSLIKE)
		    || (audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSS))
			if (em8300_audio_setup(em)) {
				return;
			}

		em8300_ioctl_enable_videoout(em, 1);

		em->ucodeloaded = 1;

		printk(KERN_NOTICE "em8300: Microcode version 0x%02x loaded\n", read_ucregister(MicroCodeVersion));

	}
}

#else

void em8300_require_ucode(struct em8300_s *em)
{
	return;
}

#endif
