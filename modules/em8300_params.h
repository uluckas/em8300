/*
 * em8300_params.h -- parameters for the em8300 driver
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

#ifndef _EM8300_PARAMS_H
#define _EM8300_PARAMS_H

#include <linux/version.h>

/* Major number used by the driver */
extern int major;

/* Card-model-dependant parameters */
extern int use_bt865[];
extern int dicom_other_pal[];
extern int dicom_fix[];
extern int dicom_control[];
extern int bt865_ucode_timeout[];
extern int activate_loopback[];

extern int card_model[];

/* Audio driver used by the driver:
   - OSS     means OSS-compatible /dev/em8300_ma + /dev/dsp (old behavior)
   - OSSLIKE means OSS-compatible /dev/em8300_ma only
   - ALSA    means ALSA /dev/snd
   - NONE    means no sound
*/
typedef enum {
	AUDIO_DRIVER_NONE,
	AUDIO_DRIVER_OSSLIKE,
	AUDIO_DRIVER_OSS,
	AUDIO_DRIVER_ALSA,
	AUDIO_DRIVER_MAX
} audio_driver_t;

extern audio_driver_t audio_driver_nr[];

/* Number of the /dev/dsp device to allocate (only used with the OSS audio
   driver.
*/
#if defined(CONFIG_SOUND) || defined(CONFIG_SOUND_MODULE)
extern int dsp_num[];
#endif

/* Number and name of the ALSA card to allocate (only used with the ALSA audio
   driver.
*/
#if defined(CONFIG_SND) || defined(CONFIG_SND_MODULE)
extern int alsa_index[];
extern char *alsa_id[];
#endif

/* Option to disable the video output when there is nothing to display */
extern int stop_video[];

/* Initialization function */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
extern void em8300_params_init(void);
#else
static inline void em8300_params_init(void)
{
}
#endif

#endif /* _EM8300_PARAMS_H */
