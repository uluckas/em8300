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
#include <linux/soundcard.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/i2c-algo-bit.h>

#include "em8300_reg.h"
#include <linux/em8300.h>
#include "em8300_fifo.h"

#ifndef AFMT_AC3
#define AFMT_AC3 0x00000400
#endif

#include <endian.h>

int em8300_audio_calcbuffered(struct em8300_s *em);
static int set_audiomode(struct em8300_s *em, int mode);

/* C decompilation of sub_prepare_SPDIF by 
*  Anton Altaparmakov <antona@bigfoot.com>
*
*  If outblock == inblock generate mute pattern.
*
* Notes:
*  local1 = "in" = current inblock position pointer.
*  local3 = "i" = for loop counter.
*  Need unsigned everywhere, otherwise get into trouble with signed shift rights!
*/

void sub_prepare_SPDIF(struct em8300_s *em, unsigned char *outblock, unsigned char *inblock, unsigned int inlength)
{
	// 	ebp-4 = local1 = in	ebp-8 = local2
	//	ebp-0ch = local3 = i	ebp-10h = local4

	unsigned char *in; // 32 bit points to array of 8 bit chars
	unsigned short int local2; // 16 bit, unsigned
	unsigned int i; // 32 bit, signed
	unsigned char local4; // 8 bit, unsigned
	unsigned char mutepattern[] = {0x00, 0x00};
	int mute;

	mute = (inblock == outblock) ? 1 : 0;
	in = (mute) ? mutepattern : inblock;

	for (i = 0; i < (inlength >> 2); i++) {
		if (em->dword_DB4 == 0xc0) {
			em->dword_DB4 = 0;
		}
		
		{
			register int ebx;
			if (em->dword_DB4 < 0) {
				ebx = em->byte_D90[(em->dword_DB4 + 7) >> 3] & 0xff << (!((!em->dword_DB4 + 1) & 7) + 1) & 0xff;
			} else {
				ebx = (em->byte_D90[em->dword_DB4 >> 3] & 0xff) << (em->dword_DB4 & 7);
			}
			local4 = (unsigned char) ((ebx & 0x80) >> 1);
		}

		local2 = in[0] << 8 | in[1];
		if (!mute)
			in +=2;

		if (em->dword_DB4 != 0) {
			outblock[i * 8 + 3] = 2;
		} else {
			outblock[i * 8 + 3] = 0;
		}

		outblock[i * 8 + 2] = local2 << 4;
		outblock[i * 8 + 1] = local2 >> 4;
		outblock[i * 8] = local2 >> 12 | local4;

       		local2 = in[0] << 8 | in[1];
		if (!mute)
			in +=2;

		outblock[i * 8 + 7] = 1;
		outblock[i * 8 + 6] = local2 << 4;
		outblock[i * 8 + 5] = local2 >> 4;
		outblock[i * 8 + 4] = local2 >> 12 | local4;

		++em->dword_DB4;
	}
	
	return;
}

static void preprocess_analog(struct em8300_s *em, unsigned char *outbuf, const unsigned char *inbuf_user, int inlength)
{
	int i;
	
#if BYTE_ORDER == BIG_ENDIAN
	if (em->audio.format == AFMT_S16_BE) {
#else /* BYTE_ORDER == LITTLE_ENDIAN */
	if (em->audio.format == AFMT_S16_LE ||
	    em->audio_mode == EM8300_AUDIOMODE_DIGITALAC3) {
#endif
		if (em->audio.channels == 2) {
			for (i = 0; i < inlength; i += 4) {
				get_user(outbuf[i + 3], inbuf_user++);
				get_user(outbuf[i + 2], inbuf_user++);
				get_user(outbuf[i + 1], inbuf_user++);
				get_user(outbuf[i], inbuf_user++);
			}
		} else {
			for (i = 0; i < inlength; i += 2) {
				get_user(outbuf[2 * i + 1], inbuf_user++);
				get_user(outbuf[2 * i], inbuf_user++);
				outbuf[2 * i + 3] = outbuf[2 * i + 1];
				outbuf[2 * i + 2] = outbuf[2 * i];
			}
		}
	} else {
		for (i = 0; i < inlength / 2; i++) {
			outbuf[2 * i] = inbuf_user[i];
			outbuf[2 * i + 1] = inbuf_user[i];
		}	
	}
}

static void preprocess_digital(struct em8300_s *em, unsigned char *outbuf,
			       const unsigned char *inbuf_user, int inlength)
{
	int i;

	if (!em->mafifo->preprocess_buffer)
		return;

#if BYTE_ORDER == BIG_ENDIAN
	if (em->audio.format == AFMT_S16_BE) {
#else /* BYTE_ORDER == LITTLE_ENDIAN */
        if (em->audio.format == AFMT_S16_LE ||
	    em->audio_mode == EM8300_AUDIOMODE_DIGITALAC3) {
#endif
		for(i = 0; i < inlength; i += 2) {
			get_user(em->mafifo->preprocess_buffer[i + 1], inbuf_user++);
			get_user(em->mafifo->preprocess_buffer[i], inbuf_user++);
		}
	} else {
		copy_from_user(em->mafifo->preprocess_buffer, inbuf_user, inlength);
	}

	sub_prepare_SPDIF(em,outbuf, em->mafifo->preprocess_buffer, inlength);
}

static void setup_mafifo(struct em8300_s *em)
{
	if (em->audio_mode == EM8300_AUDIOMODE_ANALOG) {
		em->mafifo->preprocess_ratio = ((em->audio.channels == 2) ? 1 : 2);
		em->mafifo->preprocess_cb = &preprocess_analog;
	} else {
		em->mafifo->preprocess_ratio = 2;
		em->mafifo->preprocess_cb = &preprocess_digital;
	}
}

int mpegaudio_command(struct em8300_s *em, int cmd)
{
	em8300_waitfor(em, ucregister(MA_Command), 0xffff, 0xffff);

	pr_debug("MA_Command: %d\n", cmd);
	write_ucregister(MA_Command, cmd);

	return em8300_waitfor(em, ucregister(MA_Status), cmd, 0xffff);
}

static int audio_start(struct em8300_s *em)
{
	em->irqmask |= IRQSTATUS_AUDIO_FIFO;
	write_ucregister(Q_IrqMask, em->irqmask);
	em->audio.enable_bits = PCM_ENABLE_OUTPUT;
	return mpegaudio_command(em, MACOMMAND_PLAY);
}

static int audio_stop(struct em8300_s *em)
{
	em->irqmask &= ~IRQSTATUS_AUDIO_FIFO;
	write_ucregister(Q_IrqMask, em->irqmask);
	em->audio.enable_bits = 0;
	return mpegaudio_command(em, MACOMMAND_STOP);
}

static int set_speed(struct em8300_s *em, int speed)
{
	em->clockgen &= ~CLOCKGEN_SAMPFREQ_MASK;

	switch (speed) {
	case 48000:
		em->clockgen |= CLOCKGEN_SAMPFREQ_48;
		break;
	case 44100:
		em->clockgen |= CLOCKGEN_SAMPFREQ_44;
		break;
	case 66000:
		em->clockgen |= CLOCKGEN_SAMPFREQ_66;
		break;
	case 32000:
		em->clockgen |= CLOCKGEN_SAMPFREQ_32;
		break;
	default:
		em->clockgen |= CLOCKGEN_SAMPFREQ_48;
		speed = 48000;
	}
	
	em->audio.speed = speed;

	em8300_clockgen_write(em, em->clockgen);
	
	return speed;
}

static int set_channels(struct em8300_s *em, int val)
{
	if (val > 2) val = 2;
	em->audio.channels = val;
	setup_mafifo(em);

	return val;
}

static int set_format(struct em8300_s *em, int fmt)
{
	if (fmt != AFMT_QUERY) {
		switch (fmt) {
#ifdef AFMT_AC3
		case AFMT_AC3:
			if (em->audio_mode != EM8300_AUDIOMODE_DIGITALAC3)
            {
			    set_speed(em, 48000);
				set_audiomode(em, EM8300_AUDIOMODE_DIGITALAC3);
                setup_mafifo(em);
            }
			em->audio.format = fmt;
			break;
#endif
		case AFMT_S16_BE:
		case AFMT_S16_LE:
			if (em->audio_mode == EM8300_AUDIOMODE_DIGITALAC3)
            {
				set_audiomode(em, em->pcm_mode);
                setup_mafifo(em);
            }
			em->audio.format = fmt;
			break;
		default:
			if (em->audio_mode == EM8300_AUDIOMODE_DIGITALAC3)
            {
				set_audiomode(em, em->pcm_mode);
                setup_mafifo(em);
            }
			fmt = AFMT_S16_BE;
			break;
		}
	}
	return em->audio.format;
}

int em8300_audio_ioctl(struct em8300_s *em,unsigned int cmd, unsigned long arg)
{
	int err, len = 0;
	int val = 0;

	if (_SIOC_DIR(cmd) != _SIOC_NONE && _SIOC_DIR(cmd) != 0) {
		/*
		 * Have to validate the address given by the process.
		 */
		len = _SIOC_SIZE(cmd);
		if (len < 1 || len > 65536 || arg == 0) {
			return -EFAULT;
		}
		if (_SIOC_DIR(cmd) & _SIOC_WRITE) {
			if ((err = verify_area(VERIFY_READ, (void *) arg, len)) < 0) {
				return err;
			}
		}
		if (_SIOC_DIR(cmd) & _SIOC_READ) {
			if ((err = verify_area(VERIFY_WRITE, (void *) arg, len)) < 0) {
				return err;
			}
		}
	}

	switch (cmd) { 
	case SNDCTL_DSP_RESET: /* reset device */
		pr_debug("em8300_audio.o: SNDCTL_DSP_RESET\n");
		em8300_fifo_sync(em->mafifo);
		val = 0;
		break;

	case SNDCTL_DSP_SYNC:  /* wait until last byte is played and reset device */
		pr_debug("em8300_audio.o: SNDCTL_DSP_SYNC\n");
		em8300_fifo_sync(em->mafifo);
		val = 0;
		break;

	case SNDCTL_DSP_SPEED: /* set sample rate */
		if (get_user(val, (int *) arg)) {
			return -EFAULT;
		}
		pr_debug("em8300_audio.o: SNDCTL_DSP_SPEED %i ", val);
		val = set_speed(em, val);
		pr_debug("%i\n", val);
		break;

	case SOUND_PCM_READ_RATE: /* read sample rate */
		pr_debug("em8300_audio.o: SNDCTL_DSP_RATE %i ", val);
		val = em->audio.speed;
		pr_debug("%i\n", val);
		break;

	case SNDCTL_DSP_STEREO: /* set stereo or mono mode */
		if (get_user(val, (int *) arg)) {
			return -EFAULT;
		}
		if (val > 1 || val < 0) {
			return -EINVAL;
		}
		pr_debug("em8300_audio.o: SNDCTL_DSP_STEREO %i\n", val);
		set_channels(em, val + 1);
		break;

	case SNDCTL_DSP_GETBLKSIZE: /* get fragment size */
		val = em->audio.slotsize;
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETBLKSIZE %i\n", val);
		break;

	case SNDCTL_DSP_CHANNELS: /* set number of channels */
		if (get_user(val, (int *) arg)) {
			return -EFAULT;
		}
		if (val > 2 || val < 1) {
			return -EINVAL;
		}
		pr_debug("em8300_audio.o: SNDCTL_DSP_CHANNELS %i\n", val);
		set_channels(em, val);
		break;

	case SOUND_PCM_READ_CHANNELS: /* read number of channels */
		val = em->audio.channels;
		pr_debug("em8300_audio.o: SOUND_PCM_READ_CHANNELS %i\n", val);
		break;

	case SNDCTL_DSP_POST: /* "there is likely to be a pause in the output" */
		pr_debug("em8300_audio.o: SNDCTL_DSP_POST\n");
		pr_info("em8300_audio.o: SNDCTL_DSP_GETPOST not implemented yet\n");
		return -ENOSYS;
		break;

	case SNDCTL_DSP_SETFRAGMENT: /* set fragment size */
		pr_debug("em8300_audio.o: SNDCTL_DSP_SETFRAGMENT %i\n", val);
		pr_info("em8300_audio.o: SNDCTL_DSP_SETFRAGMENT not implemented yet\n");
		break;

	case SNDCTL_DSP_GETFMTS: /* get possible formats */
#ifdef AFMT_AC3
		val = AFMT_AC3 | AFMT_S16_BE | AFMT_S16_LE;
#else
		val = AFMT_S16_BE | AFMT_S16_LE;
#endif
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETFMTS\n");
		break;

	case SNDCTL_DSP_SETFMT: /* set sample format */
		if (get_user(val, (int *) arg)) {
			return -EFAULT;
		}
		pr_debug("em8300_audio.o: SNDCTL_DSP_SETFMT %i ", val);
		val = set_format(em, val);
		pr_debug("%i\n", val);
		break;

	case SOUND_PCM_READ_BITS: /* read sample format */
		val = em->audio.format;
		pr_debug("em8300_audio.o: SOUND_PCM_READ_BITS\n");
		break;

	case SNDCTL_DSP_GETOSPACE:
	{
		audio_buf_info buf_info;
		switch(em->audio_mode)
		{
			case EM8300_AUDIOMODE_ANALOG:
				buf_info.fragments=
					em8300_fifo_freeslots(em->mafifo) -
					em->mafifo->nslots / 2;
				break;
			default:
				buf_info.fragments=
					em8300_fifo_freeslots(em->mafifo) / 2;
				break;
		}
		buf_info.fragments = (buf_info.fragments > 0) ? buf_info.fragments : 0;
		buf_info.fragstotal = em->mafifo->nslots / 2;
		buf_info.fragsize = em->audio.slotsize;
		buf_info.bytes = em->mafifo->nslots * em->audio.slotsize / 2;
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETOSPACE\n");
		return copy_to_user((void *) arg, &buf_info, sizeof(audio_buf_info));
	}
	
	case SNDCTL_DSP_GETISPACE:
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETISPACE\n");
		return -ENOSYS;
		break;

	case SNDCTL_DSP_GETCAPS:
		val = DSP_CAP_REALTIME | DSP_CAP_BATCH | DSP_CAP_TRIGGER;
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETCAPS\n");
		break;

	case SNDCTL_DSP_GETTRIGGER:
		val = em->audio.enable_bits;
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETTRIGGER\n");
		break;

	case SNDCTL_DSP_SETTRIGGER:
		if (val & PCM_ENABLE_OUTPUT) {
			if (em->audio.enable_bits & PCM_ENABLE_OUTPUT) {
				em->audio.enable_bits |= PCM_ENABLE_OUTPUT;
				mpegaudio_command(em, MACOMMAND_PLAY);
			}
		}
		pr_debug("em8300_audio.o: SNDCTL_DSP_SETTRIGGER\n");
		pr_info("em8300_audio.o: SNDCTL_DSP_SETTRIGGER not implemented properly yet\n");
		break;

	case SNDCTL_DSP_GETIPTR:
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETIPTR\n");
		return -ENOSYS;
		break;

	case SNDCTL_DSP_GETOPTR:
	{
		count_info ci;
		ci.bytes = em->mafifo->bytes - em8300_audio_calcbuffered(em);
		if (ci.bytes < 0) ci.bytes = 0;
		ci.blocks = 0;
		ci.ptr = 0;
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETOPTR %i\n", ci.bytes);
		return copy_to_user((void *) arg, &ci, sizeof(count_info));
	}
	case SNDCTL_DSP_GETODELAY:
		val = em8300_audio_calcbuffered(em);
		pr_debug("em8300_audio.o: SNDCTL_DSP_GETODELAY %i\n", val);
		break;

	default:
		pr_info("em8300_audio.o: unknown ioctl called\n");
		return -EINVAL;
	}

	return put_user(val, (int *) arg);
}

int em8300_audio_flush(struct em8300_s *em)
{
	int audiobuf, audiobufsize;

	mpegaudio_command(em, MACOMMAND_STOP);

	/* Zero audio buffer */
	audiobuf = read_ucregister(MA_BuffStart_Lo) | (read_ucregister(MA_BuffStart_Hi) << 16);
	audiobufsize = read_ucregister(MA_BuffSize) | (read_ucregister(MA_BuffSize_Hi) << 16);

	mpegaudio_command(em, MACOMMAND_PAUSE);
	
	return em8300_setregblock(em, audiobuf, 0, audiobufsize);
}

int em8300_audio_open(struct em8300_s *em)
{
	if (!em->ucodeloaded) {
		return -ENODEV;
	}
	
	em->mafifo->bytes = 0;

	return audio_start(em);
}

int em8300_audio_release(struct em8300_s *em)
{
	return audio_stop(em);	
}

static int set_audiomode(struct em8300_s *em, int mode)
{
	em->audio_mode = mode;
	
	em->clockgen &= ~CLOCKGEN_OUTMASK;

	if (em->audio_mode == EM8300_AUDIOMODE_ANALOG) {
		em->clockgen |= CLOCKGEN_ANALOGOUT;
	} else {
		em->clockgen |= CLOCKGEN_DIGITALOUT;
	}
	
	em8300_clockgen_write(em, em->clockgen);

	memset(em->byte_D90, 0, sizeof(em->byte_D90));

	em->byte_D90[1] = 0x98;

	switch (em->audio.speed) {
	case 32000:
		em->byte_D90[3] = 0xc0;
		break;
	case 44100:
		em->byte_D90[3] = 0;
		break;
	case 48000:
		em->byte_D90[3] = 0x40;
		break;
	}
	 
	switch (em->audio_mode) {
	case EM8300_AUDIOMODE_ANALOG:
		em->pcm_mode = EM8300_AUDIOMODE_ANALOG;

		write_register(EM8300_AUDIO_RATE, 0x62);
		em8300_setregblock(em, 2 * ucregister(Mute_Pattern), 0, 0x600);
		printk(KERN_NOTICE "em8300_audio.o: Analog audio enabled\n");
		break;
	case EM8300_AUDIOMODE_DIGITALPCM:
		em->pcm_mode = EM8300_AUDIOMODE_DIGITALPCM;

		write_register(EM8300_AUDIO_RATE, 0x3a0);

		em->byte_D90[0] = 0x0;
        sub_prepare_SPDIF(em, em->mafifo->preprocess_buffer, em->mafifo->preprocess_buffer, 0x300);

        em8300_writeregblock(em, 2*ucregister(Mute_Pattern), (unsigned *)em->mafifo->preprocess_buffer, em->mafifo->slotsize);

		printk(KERN_NOTICE "em8300_audio.o: Digital PCM audio enabled\n");
		break;
	case EM8300_AUDIOMODE_DIGITALAC3:
		write_register(EM8300_AUDIO_RATE, 0x3a0);

		em->byte_D90[0] = 0x40;
        sub_prepare_SPDIF(em, em->mafifo->preprocess_buffer, em->mafifo->preprocess_buffer, 0x300);

        em8300_writeregblock(em, 2*ucregister(Mute_Pattern), (unsigned *)em->mafifo->preprocess_buffer, em->mafifo->slotsize);
		printk(KERN_NOTICE "em8300_audio.o: Digital AC3 audio enabled\n");
		break;
	}
	return 0;
}

int em8300_audio_setup(struct em8300_s *em)
{
	int ret;

	em->audio.channels = 2;
	em->audio.format = AFMT_S16_NE;
	em->audio.slotsize = em->mafifo->slotsize;

	em->clockgen = em->clockgen_tvmode;

	set_speed(em, 48000);
	
	set_audiomode(em, EM8300_AUDIOMODE_DEFAULT);
	
	ret = em8300_audio_flush(em);

	setup_mafifo(em);
	
	if (ret) {
		printk(KERN_ERR "em8300_audio.o: Couldn't zero audio buffer\n");
		return ret;
	}
	
	write_ucregister(MA_Threshold, 6);
	
	mpegaudio_command(em, MACOMMAND_PLAY);
	mpegaudio_command(em, MACOMMAND_PAUSE);

	em->audio.enable_bits = 0;

	return 0;
}

int em8300_audio_calcbuffered(struct em8300_s *em)
{
	int readptr, writeptr, bufsize, n;

	readptr = read_ucregister(MA_Rdptr) | (read_ucregister(MA_Rdptr_Hi) << 16);
	writeptr = read_ucregister(MA_Wrptr) | (read_ucregister(MA_Wrptr_Hi) << 16);
	bufsize = read_ucregister(MA_BuffSize) | (read_ucregister(MA_BuffSize_Hi) << 16);

	n = ((bufsize+writeptr-readptr) % bufsize);

	return (em8300_fifo_calcbuffered(em->mafifo) + n) /
		em->mafifo->preprocess_ratio;
}

int em8300_audio_write(struct em8300_s *em, const char * buf, size_t count, loff_t *ppos)
{
	if (em->nonblock[1]) {
		return em8300_fifo_write(em->mafifo, count, buf, 0);
	} else {
		return em8300_fifo_writeblocking(em->mafifo, count, buf, 0);
	}
}

/* 18-09-2000 - Ze'ev Maor - added these two ioctls to set and get audio mode. */

int em8300_ioctl_setaudiomode(struct em8300_s *em, int mode)
{
	em8300_audio_flush(em);
	set_audiomode(em, mode);
	setup_mafifo(em);
	mpegaudio_command(em, MACOMMAND_PLAY);
	em->audio.enable_bits = PCM_ENABLE_OUTPUT;
	return 0;
}

int em8300_ioctl_getaudiomode(struct em8300_s *em, int mode)
{
	int a = em->audio_mode;
	copy_to_user((void *) mode, &a, sizeof(int));
	return 0;
}
