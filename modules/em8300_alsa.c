/* $Id$
 *
 * em8300_alsa.c -- alsa interface to the audio part of the em8300 chip
 * Copyright (C) 2004 Nicolas Boullis <nboullis@debian.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "em8300_alsa.h"

#if 1

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/control.h>
#include <linux/em8300.h>
#include <linux/pci.h>
#include <linux/stringify.h>

#include "em8300_reg.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
#define snd_magic_kcalloc(type, extra, flags) (type *) kcalloc(1, sizeof(type) + extra, flags)
#define snd_magic_kmalloc(type, extra, flags) (type *) kmalloc(sizeof(type) + extra, flags)
#define snd_magic_cast(type, ptr, retval) (type *) ptr
#define snd_magic_cast1(type, ptr, retval) snd_magic_cast(type, ptr, retval)
#define snd_magic_kfree kfree
#endif

#define chip_t em8300_alsa_t

typedef struct {
	struct em8300_s *em;
	snd_card_t *card;
	snd_pcm_t *pcm_analog;
	snd_pcm_t *pcm_digital;
	snd_pcm_substream_t *substream;
	int volume[2];
} em8300_alsa_t;

#define em8300_alsa_t_magic 0x11058300

#define EM8300_ALSA_ANALOG_DEVICENUM 0
#define EM8300_ALSA_DIGITAL_DEVICENUM 1

static snd_pcm_hardware_t snd_em8300_playback_hw = {
	.info = (
		 SNDRV_PCM_INFO_MMAP |
		 SNDRV_PCM_INFO_INTERLEAVED |
		 //		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 //		 SNDRV_PCM_INFO_MMAP_VALID |
		 SNDRV_PCM_INFO_PAUSE |
		 0),
	.rates = SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
	.rate_min = 32000,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 2,
};

static int snd_em8300_playback_open(snd_pcm_substream_t *substream)
{
	em8300_alsa_t *em8300_alsa = snd_pcm_substream_chip(substream);
	struct em8300_s *em = em8300_alsa->em;
	snd_pcm_runtime_t *runtime = substream->runtime;

	em8300_require_ucode(em);

	em8300_alsa->substream = substream;

	if (substream->pcm->device == EM8300_ALSA_ANALOG_DEVICENUM)
		snd_em8300_playback_hw.formats = SNDRV_PCM_FMTBIT_S16_BE;
	else
		snd_em8300_playback_hw.formats = SNDRV_PCM_FMTBIT_IEC958_SUBFRAME_BE;
	snd_em8300_playback_hw.periods_min = read_ucregister(MA_PCISize) / 3;
	snd_em8300_playback_hw.periods_max = read_ucregister(MA_PCISize) / 3;
	snd_em8300_playback_hw.period_bytes_min = 4096;
	snd_em8300_playback_hw.period_bytes_max = 8192;
	snd_em8300_playback_hw.buffer_bytes_max = snd_em8300_playback_hw.periods_max * snd_em8300_playback_hw.period_bytes_max;
	runtime->hw = snd_em8300_playback_hw;

//	printk("snd_em8300_playback_open called.\n");

	em->clockgen &= ~CLOCKGEN_OUTMASK;
	if (substream->pcm->device == EM8300_ALSA_ANALOG_DEVICENUM)
		em->clockgen |= CLOCKGEN_ANALOGOUT;
	else
		em->clockgen |= CLOCKGEN_DIGITALOUT;
	em8300_clockgen_write(em, em->clockgen);

	if (substream->pcm->device == EM8300_ALSA_ANALOG_DEVICENUM) {
		write_register(EM8300_AUDIO_RATE, 0x62);
		em8300_setregblock(em, 2 * ucregister(Mute_Pattern), 0, 0x600);
	}
	else {
		write_register(EM8300_AUDIO_RATE, 0x3a0);
	}

	return 0;
}

static int snd_em8300_playback_close(snd_pcm_substream_t *substream)
{
	em8300_alsa_t *em8300_alsa = snd_pcm_substream_chip(substream);

	em8300_alsa->substream = NULL;
//	To be continued.
//	printk("snd_em8300_playback_close called.\n");
	return 0;
}

static int snd_em8300_pcm_hw_params(snd_pcm_substream_t *substream, snd_pcm_hw_params_t *hw_params)
{
//	printk("snd_em8300_pcm_hw_params called.\n");
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

static int snd_em8300_pcm_hw_free(snd_pcm_substream_t *substream)
{
//	printk("snd_em8300_pcm_hw_free called.\n");
	return snd_pcm_lib_free_pages(substream);
}

static int snd_em8300_pcm_prepare(snd_pcm_substream_t *substream)
{
	em8300_alsa_t *em8300_alsa = snd_pcm_substream_chip(substream);
	struct em8300_s *em = em8300_alsa->em;
	snd_pcm_runtime_t *runtime = substream->runtime;
	int i;
	dma_addr_t phys;
//	printk("snd_em8300_pcm_prepare called.\n");

	em->clockgen &= ~CLOCKGEN_SAMPFREQ_MASK;
	switch (runtime->rate) {
	case 48000:
//		printk("runtime->rate set to 48000\n");
		em->clockgen |= CLOCKGEN_SAMPFREQ_48;
		break;
	case 44100:
//		printk("runtime->rate set to 44100\n");
		em->clockgen |= CLOCKGEN_SAMPFREQ_44;
		break;
	case 32000:
//		printk("runtime->rate set to 32000\n");
		em->clockgen |= CLOCKGEN_SAMPFREQ_32;
		break;
	default:
//		printk("bad runtime->rate\n");
		em->clockgen |= CLOCKGEN_SAMPFREQ_48;
	}
	em8300_clockgen_write(em, em->clockgen);

	for (i = 0; i < runtime->periods; i++) {
		phys = runtime->dma_addr + i * frames_to_bytes(runtime, runtime->period_size);
		writel(phys >> 16, ((uint32_t *)ucregister_ptr(MA_PCIStart))+3*i);
		writel(phys & 0xffff, ((uint32_t *)ucregister_ptr(MA_PCIStart))+3*i+1);
		writel(frames_to_bytes(runtime, runtime->period_size), ((uint32_t *)ucregister_ptr(MA_PCIStart))+3*i+2);
	}
	write_ucregister(MA_PCIRdPtr, ucregister(MA_PCIStart) - 0x1000);
	write_ucregister(MA_PCIWrPtr, ucregister(MA_PCIStart) - 0x1000 + 3 * (runtime->periods - 1));

	return 0;
}

static int snd_em8300_pcm_trigger(snd_pcm_substream_t *substream, int cmd)
{
	em8300_alsa_t *em8300_alsa = snd_pcm_substream_chip(substream);
	struct em8300_s *em = em8300_alsa->em;
	snd_pcm_runtime_t *runtime = substream->runtime;
//	printk("snd_em8300_pcm_trigger(%d) called.\n", cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		em->irqmask |= IRQSTATUS_AUDIO_FIFO;
		write_ucregister(Q_IrqMask, em->irqmask);
		{
			int readindex = ((int)read_ucregister(MA_PCIRdPtr) - (ucregister(MA_PCIStart) - 0x1000)) / 3;
			int writeindex = (readindex + runtime->periods - 1) % runtime->periods;
			write_ucregister(MA_PCIWrPtr, ucregister(MA_PCIStart) - 0x1000 + writeindex * 3);
		}
		mpegaudio_command(em, MACOMMAND_PLAY);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		em->irqmask &= ~IRQSTATUS_AUDIO_FIFO;
		write_ucregister(Q_IrqMask, em->irqmask);
		mpegaudio_command(em, MACOMMAND_STOP);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		mpegaudio_command(em, MACOMMAND_PAUSE);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		mpegaudio_command(em, MACOMMAND_PLAY);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static snd_pcm_uframes_t snd_em8300_pcm_pointer(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	em8300_alsa_t *em8300_alsa = snd_pcm_substream_chip(substream);
	struct em8300_s *em = em8300_alsa->em;
	int readindex = ((int)read_ucregister(MA_PCIRdPtr) - (ucregister(MA_PCIStart) - 0x1000)) / 3;
	ssize_t pos = readindex * frames_to_bytes(runtime, runtime->period_size);
//	printk("snd_em8300_pcm_pointer called.\n");
	return bytes_to_frames(runtime, pos);
}


static snd_pcm_ops_t snd_em8300_playback_ops = {
	.open =		snd_em8300_playback_open,
	.close =	snd_em8300_playback_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_em8300_pcm_hw_params,
	.hw_free =	snd_em8300_pcm_hw_free,
	.prepare =	snd_em8300_pcm_prepare,
	.trigger =	snd_em8300_pcm_trigger,
	.pointer =	snd_em8300_pcm_pointer,
};

/*
static void snd_em8300_pcm_free(snd_pcm_t *pcm)
{
	em8300_alsa_t *em8300_alsa = snd_magic_cast(em8300_alsa_t, pcm->private_data, return);
	em8300_alsa->pcm = NULL;
//	snd_pcm_lib_preallocate_free_for_all(pcm);
}
*/

static int snd_em8300_pcm_analog(em8300_alsa_t *em8300_alsa)
{
	struct em8300_s *em = em8300_alsa->em;
	snd_pcm_t *pcm;
	int err;

	if ((err = snd_pcm_new(em8300_alsa->card, "EM8300/" __stringify(EM8300_ALSA_ANALOG_DEVICENUM), EM8300_ALSA_ANALOG_DEVICENUM, 1, 0, &pcm))<0)
		return err;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_em8300_playback_ops);

	pcm->private_data = em8300_alsa;
	//	pcm->private_free = snd_em8300_pcm_free;
	//	pcm->info_flags = 0;

	strcpy(pcm->name, "EM8300 DAC");

	em8300_alsa->pcm_analog = pcm;

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_pci_data(em->dev),
					      (read_ucregister(MA_PCISize) / 3) * 32768,
					      (read_ucregister(MA_PCISize) / 3) * 32768);

	return 0;
}

static int snd_em8300_pcm_digital(em8300_alsa_t *em8300_alsa)
{
	struct em8300_s *em = em8300_alsa->em;
	snd_pcm_t *pcm;
	int err;

	if ((err = snd_pcm_new(em8300_alsa->card, "EM8300/" __stringify(EM8300_ALSA_DIGITAL_DEVICENUM), EM8300_ALSA_DIGITAL_DEVICENUM, 1, 0, &pcm))<0)
		return err;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_em8300_playback_ops);

	pcm->private_data = em8300_alsa;
	//	pcm->private_free = snd_em8300_pcm_free;
	//	pcm->info_flags = 0;

	strcpy(pcm->name, "EM8300 IEC958");

	em8300_alsa->pcm_digital = pcm;

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_pci_data(em->dev),
					      (read_ucregister(MA_PCISize) / 3) * 32768,
					      (read_ucregister(MA_PCISize) / 3) * 32768);

	return 0;
}

static int snd_em8300_free(em8300_alsa_t *em8300_alsa)
{
	snd_magic_kfree(em8300_alsa);
	return 0;
}

static int snd_em8300_dev_free(snd_device_t *device)
{
	em8300_alsa_t *em8300_alsa = snd_magic_cast(em8300_alsa_t, device->device_data, return -ENXIO);
	return snd_em8300_free(em8300_alsa);
}

static int snd_em8300_create(snd_card_t *card, struct em8300_s *em, em8300_alsa_t **rem8300_alsa)
{
	em8300_alsa_t *em8300_alsa;
	int err;
	static snd_device_ops_t ops = {
		.dev_free = snd_em8300_dev_free,
	};

	if (rem8300_alsa)
		*rem8300_alsa = NULL;

	em8300_alsa = snd_magic_kcalloc(em8300_alsa_t, 0, GFP_KERNEL);
	if (em8300_alsa == NULL)
		return -ENOMEM;

	em8300_alsa->em = em;
	em8300_alsa->card = card;

	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, em8300_alsa, &ops)) < 0) {
		snd_em8300_free(em8300_alsa);
		return err;
	}

	snd_card_set_dev(card, &em->dev->dev);

	*rem8300_alsa = em8300_alsa;
	return 0;
}

static void em8300_alsa_enable_card(struct em8300_s *em)
{
	snd_card_t *card;
	em8300_alsa_t *em8300_alsa;
	int err;

	em->alsa_card = NULL;

	card = snd_card_new(-1, NULL, THIS_MODULE, 0);
	if (card == NULL)
		return;

	if ((err = snd_em8300_create(card, em, &em8300_alsa)) < 0) {
		snd_card_free(card);
		return;
	}

	card->private_data = em8300_alsa;

	if ((err = snd_em8300_pcm_analog(em8300_alsa)) < 0) {
		snd_card_free(card);
		return;
	}

	if ((err = snd_em8300_pcm_digital(em8300_alsa)) < 0) {
		snd_card_free(card);
		return;
	}

	strcpy(card->driver, "EM8300");
	strcpy(card->shortname, "Sigma Designs' EM8300");
	sprintf(card->longname, "%s", card->shortname);

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return;
	}

	em->alsa_card = card;
}

static void em8300_alsa_disable_card(struct em8300_s *em)
{
	if (em->alsa_card)
		snd_card_free(em->alsa_card);
}

void em8300_alsa_audio_interrupt(struct em8300_s *em)
{
	int readindex = ((int)read_ucregister(MA_PCIRdPtr) - (ucregister(MA_PCIStart) - 0x1000)) / 3;
	int writeindex = ((int)read_ucregister(MA_PCIWrPtr) - (ucregister(MA_PCIStart) - 0x1000)) / 3;
	//	printk("em8300_alsa: interrupt with readindex=%d and writeindex=%d\n", readindex, writeindex);
	writeindex = (readindex + (read_ucregister(MA_PCISize)/3) - 1) % (read_ucregister(MA_PCISize)/3);
	write_ucregister(MA_PCIWrPtr, ucregister(MA_PCIStart) - 0x1000 + writeindex * 3);
	if (em->alsa_card) {
		em8300_alsa_t *em8300_alsa = snd_magic_cast(em8300_alsa_t, em->alsa_card->private_data, return -ENXIO);
		if (em8300_alsa->substream) {
//			printk("calling snd_pcm_period_elapsed\n");
			snd_pcm_period_elapsed(em8300_alsa->substream);
		}
	}
}

struct em8300_registrar_s em8300_alsa_registrar =
{
	.register_driver   = NULL,
	.register_card     = NULL,
	.enable_card       = &em8300_alsa_enable_card,
	.disable_card      = &em8300_alsa_disable_card,
	.unregister_card   = NULL,
	.unregister_driver = NULL,
	.audio_interrupt   = em8300_alsa_audio_interrupt,
	.video_interrupt   = NULL,
	.vbl_interrupt     = NULL,
};

#else

struct em8300_registrar_s em8300_alsa_registrar =
{
	.register_driver   = NULL,
	.register_card     = NULL,
	.enable_card       = NULL,
	.disable_card      = NULL,
	.unregister_card   = NULL,
	.unregister_driver = NULL,
	.audio_interrupt   = NULL,
	.video_interrupt   = NULL,
	.vbl_interrupt     = NULL,
};

#endif
