/*
 * Copyright (C) 2005 Jon Burgess <jburgess@uklinux.net>
 */
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
#include <linux/wait.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include "em8300_reg.h"
#include <linux/em8300.h>
#include "em8300_fifo.h"

#include "encoder.h"

#include "em8300_registration.h"
#include "em8300_compat24.h"

typedef enum {
	AUDIO_DRIVER_NONE,
	AUDIO_DRIVER_OSSLIKE,
	AUDIO_DRIVER_OSS,
	AUDIO_DRIVER_ALSA,
	AUDIO_DRIVER_MAX
} audio_driver_t;

extern audio_driver_t audio_driver_nr[EM8300_MAX];

int em8300_control_ioctl(struct em8300_s *em, int cmd, unsigned long arg)
{
	em8300_register_t reg;
	int val, len;
	em8300_bcs_t bcs;
	em8300_overlay_window_t ov_win;
	em8300_overlay_screen_t ov_scr;
	em8300_overlay_calibrate_t ov_cal;
	em8300_attribute_t attr;
	int old_count;
	long ret;

	if (_IOC_DIR(cmd) != 0) {
		len = _IOC_SIZE(cmd);

		if (len < 1 || len > 65536 || arg == 0) {
			return -EFAULT;
		}
		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (!access_ok(VERIFY_READ, (void *) arg, len)) {
				return -EFAULT;
			}
		}
		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (!access_ok(VERIFY_WRITE, (void *) arg, len)) {
				return -EFAULT;
			}
		}
	}

	switch(_IOC_NR(cmd)) {
	case _IOC_NR(EM8300_IOCTL_INIT):
		return em8300_ioctl_init(em, (em8300_microcode_t *) arg);

	case _IOC_NR(EM8300_IOCTL_WRITEREG):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (copy_from_user(&reg, (void *) arg, sizeof(em8300_register_t)))
			return -EFAULT;

		if (reg.microcode_register) {
			write_ucregister(reg.reg, reg.val);
		} else {
			write_register(reg.reg, reg.val);
		}
		break;

	case _IOC_NR(EM8300_IOCTL_READREG):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (copy_from_user(&reg, (void *) arg, sizeof(em8300_register_t)))
			return -EFAULT;

		if (reg.microcode_register) {
			reg.val = read_ucregister(reg.reg);
			reg.reg = ucregister(reg.reg);
		} else {
			reg.val = read_register(reg.reg);
		}
		if (copy_to_user((void *) arg, &reg, sizeof(em8300_register_t)))
			return -EFAULT;
		break;

	case _IOC_NR(EM8300_IOCTL_GETSTATUS):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		return em8300_ioctl_getstatus(em, (char *) arg);
		break;

	case _IOC_NR(EM8300_IOCTL_VBI):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		old_count = em->irqcount;
		em->irqmask |= IRQSTATUS_VIDEO_VBL;
		write_ucregister(Q_IrqMask, em->irqmask);

		ret = wait_event_interruptible_timeout(em->vbi_wait, em->irqcount != old_count, HZ);
		if (ret == 0)
			return -EINTR;
		else if (ret < 0)
		        return ret;		

		/* copy timestamp and return */
		if (copy_to_user((void *) arg, &em->tv, sizeof(struct timeval)))
			return -EFAULT;
		return 0;

	case _IOC_NR(EM8300_IOCTL_GETBCS):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (copy_from_user(&bcs, (void *) arg, sizeof(em8300_bcs_t)))
				return -EFAULT;
			em8300_dicom_setBCS(em, bcs.brightness, bcs.contrast, bcs.saturation);
		}

		if (_IOC_DIR(cmd) & _IOC_READ) {
			bcs.brightness = em->dicom_brightness;
			bcs.contrast = em->dicom_contrast;
			bcs.saturation = em->dicom_saturation;
			if (copy_to_user((void *) arg, &bcs, sizeof(em8300_bcs_t)))
				return -EFAULT;
		}
		break;

	case _IOC_NR(EM8300_IOCTL_SET_VIDEOMODE):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			get_user(val, (int *) arg);
			em8300_ioctl_setvideomode(em, val);
		}

		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void *) arg, &em->video_mode, sizeof(em->video_mode)))
				return -EFAULT;
		}
		break;

	case _IOC_NR(EM8300_IOCTL_SET_PLAYMODE):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			get_user(val, (int *) arg);
			em8300_ioctl_setplaymode(em, val);
		}
		break;

	case _IOC_NR(EM8300_IOCTL_SET_ASPECTRATIO):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			get_user(val, (int *) arg);
			em8300_ioctl_setaspectratio(em, val);
		}

		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void *) arg, &em->aspect_ratio, sizeof(em->aspect_ratio)))
				return -EFAULT;
		}
		break;
	case _IOC_NR(EM8300_IOCTL_GET_AUDIOMODE):
		if ((audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSSLIKE)
		    || (audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSS)) {
			em8300_require_ucode(em);

			if (!em->ucodeloaded) {
				return -ENOTTY;
			}

			if (_IOC_DIR(cmd) & _IOC_WRITE) {
				get_user(val, (int *) arg);
				em8300_ioctl_setaudiomode(em, val);
			}
			if (_IOC_DIR(cmd) & _IOC_READ) {
				em8300_ioctl_getaudiomode(em, arg);
			}
			break;
		}
		else
			return -EINVAL;
	case _IOC_NR(EM8300_IOCTL_SET_SPUMODE):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			get_user(val, (int *) arg);
			em8300_ioctl_setspumode(em, val);
		}

		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void *) arg, &em->sp_mode, sizeof(em->sp_mode)))
				return -EFAULT;
		}
		break;

	case _IOC_NR(EM8300_IOCTL_OVERLAY_SETMODE):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			get_user(val, (int *) arg);
			if (!em8300_ioctl_overlay_setmode(em, val)) {
				return -EINVAL;
			}
		}
		break;

	case _IOC_NR(EM8300_IOCTL_OVERLAY_SIGNALMODE):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			get_user(val, (int *) arg);
			if (!em9010_overlay_set_signalmode(em, val)) {
				return -EINVAL;
			}
		}
		break;

	case _IOC_NR(EM8300_IOCTL_OVERLAY_SETWINDOW):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (copy_from_user(&ov_win, (void *) arg, sizeof(em8300_overlay_window_t)))
				return -EFAULT;
			if (!em8300_ioctl_overlay_setwindow(em, &ov_win)) {
				return -EINVAL;
			}
		}
		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void *) arg, &ov_win, sizeof(em8300_overlay_window_t)))
				return -EFAULT;
		}
		break;

	case _IOC_NR(EM8300_IOCTL_OVERLAY_SETSCREEN):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (copy_from_user(&ov_scr, (void *) arg, sizeof(em8300_overlay_screen_t)))
				return -EFAULT;
			if (!em8300_ioctl_overlay_setscreen(em, &ov_scr)) {
				return -EINVAL;
			}
		}
		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void *) arg, &ov_scr, sizeof(em8300_overlay_screen_t)))
				return -EFAULT;
		}
	break;

	case _IOC_NR(EM8300_IOCTL_OVERLAY_CALIBRATE):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (copy_from_user(&ov_cal, (void *) arg, sizeof(em8300_overlay_calibrate_t)))
				return -EFAULT;
			if(!em8300_ioctl_overlay_calibrate(em, &ov_cal)) {
				return -EIO;
			}
		}

		if (_IOC_DIR(cmd) & _IOC_READ) {
			if (copy_to_user((void *) arg, &ov_cal, sizeof(em8300_overlay_calibrate_t)))
				return -EFAULT;
		}
	break;

	case _IOC_NR(EM8300_IOCTL_OVERLAY_GET_ATTRIBUTE):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (copy_from_user(&attr, (void *) arg, sizeof(em8300_attribute_t)))
			return -EFAULT;
		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			em9010_set_attribute(em, attr.attribute, attr.value);
		}
		if (_IOC_DIR(cmd) & _IOC_READ) {
			attr.value = em9010_get_attribute(em, attr.attribute);
			if (copy_to_user((void *) arg, &attr, sizeof(em8300_attribute_t)))
				return -EFAULT;
		}
		break;

	case _IOC_NR(EM8300_IOCTL_SCR_GET):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			unsigned scr;
			if (get_user(val, (unsigned*) arg))
				return -EFAULT;
			scr = read_ucregister(MV_SCRlo) | (read_ucregister(MV_SCRhi) << 16);

			if (scr > val)
				scr = scr - val;
			else
				scr = val - scr;

			if (scr > 2 * 1800) { /* Tolerance: 2 frames */
				pr_info("adjusting scr: %i\n", val);
				write_ucregister(MV_SCRlo, val & 0xffff);
				write_ucregister(MV_SCRhi, (val >> 16) & 0xffff);
			}
		}
		if (_IOC_DIR(cmd) & _IOC_READ) {
			val = read_ucregister(MV_SCRlo) | (read_ucregister(MV_SCRhi) << 16);
			if (copy_to_user((void *) arg, &val, sizeof(unsigned)))
				return -EFAULT;
		}
	break;

	case _IOC_NR(EM8300_IOCTL_SCR_GETSPEED):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			get_user(val, (int*) arg);
			val &= 0xFFFF;

			write_ucregister(MV_SCRSpeed,
			 read_ucregister(MicroCodeVersion) >= 0x29 ? val : val >> 8);
		}
		if (_IOC_DIR(cmd) & _IOC_READ) {
			val = read_ucregister(MV_SCRSpeed);
			if (! read_ucregister(MicroCodeVersion) >= 0x29)
				val <<= 8;

			if (copy_to_user((void *) arg, &val, sizeof(unsigned)))
				return -EFAULT;
		}
	break;

	case _IOC_NR(EM8300_IOCTL_FLUSH):
		em8300_require_ucode(em);

		if (!em->ucodeloaded) {
			return -ENOTTY;
		}

		if (_IOC_DIR(cmd) & _IOC_WRITE) {
			if (get_user(val, (unsigned*) arg))
				return -EFAULT;

			switch (val) {
			case EM8300_SUBDEVICE_CONTROL:
				return -ENOSYS;
			case EM8300_SUBDEVICE_VIDEO:
				return em8300_video_flush(em);
			case EM8300_SUBDEVICE_AUDIO:
				if ((audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSSLIKE)
				    || (audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSS))
					return em8300_audio_flush(em);
				else
					return -EINVAL;
			case EM8300_SUBDEVICE_SUBPICTURE:
				return -ENOSYS;
			default:
				return -EINVAL;
			}
		}
	break;

	default:
		return -ETIME;
	}

	return 0;
}

int em8300_ioctl_init(struct em8300_s *em, em8300_microcode_t *useruc)
{
	em8300_microcode_t uc;
	unsigned char *ucode;
	int ret;

	if (copy_from_user(&uc, useruc, sizeof(em8300_microcode_t)))
		return -EFAULT;

	ucode = kmalloc(uc.ucode_size, GFP_KERNEL);
	if (!ucode) {
		return -ENOMEM;
	}

	if (copy_from_user(ucode, uc.ucode, uc.ucode_size)) {
		kfree(ucode);
		return -EFAULT;
	}

	em8300_ucode_upload(em, ucode, uc.ucode_size);

	kfree(ucode);

	em8300_dicom_init(em);

	if ((ret = em8300_video_setup(em))) {
		return ret;
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
		return -ENOMEM;
	}

	if ((audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSSLIKE)
	    || (audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSS))
		if (!(em->mafifo = em8300_fifo_alloc())) {
			return -ENOMEM;
		}

	if (!(em->spfifo = em8300_fifo_alloc())) {
		return -ENOMEM;
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
		if ((ret = em8300_audio_setup(em))) {
			return ret;
		}

	em8300_ioctl_enable_videoout(em, 0);

#if ! ( defined(CONFIG_FW_LOADER) || defined(CONFIG_FW_LOADER_MODULE) )
	if (!em->ucodeloaded)
		em8300_enable_card(em);
#endif

	em->ucodeloaded = 1;

	printk(KERN_NOTICE "em8300: Microcode version 0x%02x loaded\n", read_ucregister(MicroCodeVersion));
	return 0;
}

int em8300_ioctl_getstatus(struct em8300_s *em, char *usermsg)
{
	char tmpstr[1024];
	struct timeval tv;
	long tdiff, frames, scr, picpts;
	char mvfstatus[128];
	char mafstatus[128];
	char spfstatus[128];

	em8300_fifo_statusmsg(em->mvfifo, mvfstatus);
	if ((audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSSLIKE)
	    || (audio_driver_nr[em->card_nr] == AUDIO_DRIVER_OSS))
		em8300_fifo_statusmsg(em->mafifo, mafstatus);
	em8300_fifo_statusmsg(em->spfifo, spfstatus);

	frames = (read_ucregister(MV_FrameCntHi) << 16) | read_ucregister(MV_FrameCntLo);
	picpts = (read_ucregister(PicPTSHi) << 16) |
	read_ucregister(PicPTSLo);
	scr = (read_ucregister(MV_SCRhi) << 16) | read_ucregister(MV_SCRlo);

	do_gettimeofday(&tv);
	tdiff = TIMEDIFF(tv, em->last_status_time);
	em->last_status_time = tv;
	em->irqcount = 0;
	em->frames = frames;
	em->scr = scr;
	if (copy_to_user((void *) usermsg, tmpstr, strlen(tmpstr) + 1))
		return -EFAULT;
	return 0;
}


int em8300_ioctl_setvideomode(struct em8300_s *em, int mode)
{
	long int encoder;

	switch (mode) {
	case EM8300_VIDEOMODE_PAL:
		encoder = ENCODER_MODE_PAL;
		break;
	case EM8300_VIDEOMODE_PAL60:
		encoder = ENCODER_MODE_PAL60;
		break;
	case EM8300_VIDEOMODE_NTSC:
		encoder = ENCODER_MODE_NTSC;
		break;
	default:
		return -EINVAL;
	}

	em->video_mode = mode;

	em8300_dicom_disable(em);

	if (em->encoder) {
		em->encoder->driver->command(em->encoder, ENCODER_CMD_SETMODE, (void *)encoder);
	}
	em8300_dicom_enable(em);
	em8300_dicom_update(em);

	return 0;
}

extern int stop_video[EM8300_MAX];

void em8300_ioctl_enable_videoout(struct em8300_s *em, int mode)
{
	em8300_dicom_disable(em);

	if (em->encoder) {
		em->encoder->driver->command(em->encoder,
					     ENCODER_CMD_ENABLEOUTPUT,
					     (void *)(long int)(stop_video[em->card_nr]?mode:1));
	}

	em8300_dicom_enable(em);
}


int em8300_ioctl_setaspectratio(struct em8300_s *em, int ratio)
{
	em->aspect_ratio = ratio;
	em8300_dicom_update(em);

	return 0;
}

int em8300_ioctl_setplaymode(struct em8300_s *em, int mode)
{
	switch (mode) {
	case EM8300_PLAYMODE_PLAY:
		mpegaudio_command(em, MACOMMAND_PLAY);
		if (em->playmode == EM8300_PLAYMODE_STOPPED) {
			em8300_ioctl_enable_videoout(em, 1);
		}
		em8300_video_setplaymode(em, mode);
		break;
	case EM8300_PLAYMODE_STOPPED:
		em8300_ioctl_enable_videoout(em, 0);
		em8300_video_setplaymode(em, mode);
		break;
	case EM8300_PLAYMODE_PAUSED:
		mpegaudio_command(em, MACOMMAND_PAUSE);
		em8300_video_setplaymode(em, mode);
		break;
	default:
		return -1;
	}
	em->playmode = mode;

	return 0;
}

int em8300_ioctl_setspumode(struct em8300_s *em, int mode)
{
	em->sp_mode = mode;
	return 0;
}

int em8300_ioctl_overlay_setmode(struct em8300_s *em, int val)
{
	switch (val) {
	case EM8300_OVERLAY_MODE_OFF:
		if (em->overlay_enabled) {
			em->clockgen = (em->clockgen & ~CLOCKGEN_MODEMASK) | em->clockgen_tvmode;
			em8300_clockgen_write(em, em->clockgen);
			em->overlay_enabled = 0;
			em->overlay_mode = val;
			em8300_ioctl_setvideomode(em, em->video_mode);
			em9010_overlay_update(em);
		}
		break;
	case EM8300_OVERLAY_MODE_RECTANGLE:
	case EM8300_OVERLAY_MODE_OVERLAY:
		if (!em->overlay_enabled) {
			em->clockgen = (em->clockgen & ~CLOCKGEN_MODEMASK) | em->clockgen_overlaymode;
			em8300_clockgen_write(em, em->clockgen);
			em->overlay_enabled = 1;
			em->overlay_mode = val;
			em8300_dicom_disable(em);
			em8300_dicom_enable(em);
			em8300_dicom_update(em);
			em9010_overlay_update(em);
		} else {
			em->overlay_mode = val;
			em9010_overlay_update(em);
		}
		break;
	default:
		return 0;
	}

	return 1;
}

int em8300_ioctl_overlay_setwindow(struct em8300_s *em, em8300_overlay_window_t *w)
{
	if (w->xpos < -2000 || w->xpos > 2000) {
		return 0;
	}
	if (w->ypos < -2000 || w->ypos > 2000) {
		return 0;
	}
	if (w->width <= 0 || w->width > 2000) {
		return 0;
	}
	if (w->height <= 0 || w->height > 2000) {
		return 0;
	}
	em->overlay_frame_xpos = w->xpos;
	em->overlay_frame_ypos = w->ypos;
	em->overlay_frame_width = w->width;
	em->overlay_frame_height = w->height;

	if (em->overlay_enabled) {
		sub_4288c(em, em->overlay_frame_xpos, em->overlay_frame_ypos, em->overlay_frame_width,
			em->overlay_frame_height, em->overlay_a[EM9010_ATTRIBUTE_XOFFSET],
			em->overlay_a[EM9010_ATTRIBUTE_YOFFSET], em->overlay_a[EM9010_ATTRIBUTE_XCORR], em->overlay_double_y);
	} else {
		em8300_dicom_update(em);
	}

	return 1;
}

int em8300_ioctl_overlay_setscreen(struct em8300_s *em, em8300_overlay_screen_t *s)
{
	if (s->xsize < 0 || s->xsize > 2000) {
		return 0;
	}
	if (s->ysize < 0 || s->ysize > 2000) {
		return 0;
	}

	em9010_overlay_set_res(em, s->xsize, s->ysize);
	return 1;
}
