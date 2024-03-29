/*
 * em8300_params.c -- parameters for the em8300 driver
 * Copyright (C) 2007 Nicolas Boullis <nboullis@debian.org>
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
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "em8300_params.h"
#include <linux/em8300.h>
#include "em8300_driver.h"
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "em8300_compat24.h"

int use_bt865[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(use_bt865, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(use_bt865, bool, NULL, 0444);
#endif
MODULE_PARM_DESC(use_bt865, "Set this to 1 if you have a bt865. It changes some internal register values. Defaults to 0.");

/*
 * Module params by Jonas Birmé (birme@jpl.nu)
 */
int dicom_other_pal[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(dicom_other_pal, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(dicom_other_pal, bool, NULL, 0444);
#endif
MODULE_PARM_DESC(dicom_other_pal, "If this is set, then some internal register values are swapped for PAL and NTSC. Defaults to 1.");

int dicom_fix[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(dicom_fix, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(dicom_fix, bool, NULL, 0444);
#endif
MODULE_PARM_DESC(dicom_fix, "If this is set then some internal register values are changed. Fixes green screen problems for some. Defaults to 1.");

int dicom_control[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(dicom_control, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(dicom_control, bool, NULL, 0444);
#endif
MODULE_PARM_DESC(dicom_control, "If this is set then some internal register values are changed. Fixes green screen problems for some. Defaults to 1.");

int bt865_ucode_timeout[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(bt865_ucode_timeout, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(bt865_ucode_timeout, bool, NULL, 0444);
#endif
MODULE_PARM_DESC(bt865_ucode_timeout, "Set this to 1 if you have a bt865 and get timeouts when uploading the microcode. Defaults to 0.");

int activate_loopback[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(activate_loopback, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(activate_loopback, bool, NULL, 0444);
#endif
MODULE_PARM_DESC(activate_loopback, "If you lose video after loading the modules or uploading the microcode set this to 1. Defaults to 0.");

int card_model[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(card_model, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(card_model, int, NULL, 0444);
#endif
MODULE_PARM_DESC(card_model, "Model number for the em8300-based card. -1 (default) means automatic detection; 0 means unknown model with manual setup.");

int major = EM8300_MAJOR;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(major, "i");
#else
module_param(major, int, 0444);
#endif
MODULE_PARM_DESC(major, "Major number used for the devices. "
		 "0 means automatically assigned. "
		 "Defaults to " __MODULE_STRING(EM8300_MAJOR) ".");

static const char * const audio_driver_name[] = {
	[ AUDIO_DRIVER_NONE ] = "none",
	[ AUDIO_DRIVER_OSSLIKE ] = "osslike",
	[ AUDIO_DRIVER_OSS ] = "oss",
	[ AUDIO_DRIVER_ALSA ] = "alsa",
};

#if defined(CONFIG_SND) || defined(CONFIG_SND_MODULE)
audio_driver_t audio_driver_nr[EM8300_MAX] = { [0 ... EM8300_MAX-1] = AUDIO_DRIVER_ALSA };
#elif defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
audio_driver_t audio_driver_nr[EM8300_MAX] = { [0 ... EM8300_MAX-1] = AUDIO_DRIVER_OSS };
#else
audio_driver_t audio_driver_nr[EM8300_MAX] = { [0 ... EM8300_MAX-1] = AUDIO_DRIVER_OSSLIKE };
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
static char *audio_driver[EM8300_MAX] = { [0 ... EM8300_MAX-1] = NULL };
MODULE_PARM(audio_driver, "1-" __MODULE_STRING(EM8300_MAX) "s");
#else

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
static int param_set_audio_driver_t(const char *val, const struct kernel_param *kp)
#else
static int param_set_audio_driver_t(const char *val, struct kernel_param *kp)
#endif
{
	pr_warning("em8300: %s: deprecated module parameter: all audio interfaces are now enabled\n", kp->name);
	if (val) {
		int i;
		for (i = 0; i < AUDIO_DRIVER_MAX; i++)
			if (strcmp(val, audio_driver_name[i]) == 0) {
				*(audio_driver_t *)kp->arg = i;
				return 0;
			}
	}
	printk(KERN_ERR "%s: audio_driver parameter expected\n",
	       kp->name);
	return -EINVAL;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
static int param_get_audio_driver_t(char *buffer, const struct kernel_param *kp)
#else
static int param_get_audio_driver_t(char *buffer, struct kernel_param *kp)
#endif
{
	return sprintf(buffer, "%s", audio_driver_name[*(audio_driver_t *)kp->arg]);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
struct kernel_param_ops param_ops_audio_driver_t = {
      .set = param_set_audio_driver_t,
      .get = param_get_audio_driver_t,
};
#endif

module_param_array_named(audio_driver, audio_driver_nr, audio_driver_t, NULL, 0444);
#endif
MODULE_PARM_DESC(audio_driver, "[DEPRECATED] The audio driver to use (none, osslike, oss, or alsa).");

#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
int dsp_num[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(dsp_num, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(dsp_num, int, NULL, 0444);
#endif
MODULE_PARM_DESC(dsp_num, "The /dev/dsp number to assign to the card. -1 for automatic (this is the default).");
#endif

#if defined(CONFIG_SND) || defined(CONFIG_SND_MODULE)
char *alsa_id[EM8300_MAX] = { [0 ... EM8300_MAX-1] = NULL };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(alsa_id, "1-" __MODULE_STRING(EM8300_MAX) "s");
#else
module_param_array(alsa_id, charp, NULL, 0444);
#endif
MODULE_PARM_DESC(alsa_id, "ID string for the audio part of the EM8300 chip (ALSA).");

int alsa_index[EM8300_MAX] = { [0 ... EM8300_MAX-1] = -1 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(alsa_index, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(alsa_index, int, NULL, 0444);
#endif
MODULE_PARM_DESC(alsa_index, "Index value for the audio part of the EM8300 chip (ALSA).");
#endif

int stop_video[EM8300_MAX] = { [0 ... EM8300_MAX-1] = 0 };
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
MODULE_PARM(stop_video, "1-" __MODULE_STRING(EM8300_MAX) "i");
#else
module_param_array(stop_video, bool, NULL, 0444);
#endif
MODULE_PARM_DESC(stop_video, "Set this to 1 if you want to stop video output instead of black when there is nothing to display. Defaults to 0.");


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
void em8300_params_init(void)
{
	int i;
	for (i = 0; i < EM8300_MAX; i++)
		if ((audio_driver[i]) && (audio_driver[i][0])) {
			int j;
			pr_warning("em8300: audio_driver: deprecated module parameter: all audio interfaces are now enabled\n");
			for (j = 0; j < AUDIO_DRIVER_MAX; j++)
				if (strcmp(audio_driver[i], audio_driver_name[j]) == 0) {
					audio_driver_nr[i] = j;
					break;
				}
			if (j == AUDIO_DRIVER_MAX)
				printk(KERN_WARNING "em8300.o: Unknown audio driver: %s\n", audio_driver[i]);
		}
}
#endif
