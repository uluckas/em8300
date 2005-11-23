/*
   ADV7175A - Analog Devices ADV7175A video encoder driver version 0.0.3

   Copyright (C) 2000 Henrik Johannson <henrikjo@post.utfors.se>

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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/signal.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <linux/types.h>

#include <linux/videodev.h>
#include <linux/version.h>
#include <asm/uaccess.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/video_encoder.h>

#include "em8300_compat24.h"
#include "em8300_reg.h"
#include <linux/em8300.h>

#include "adv717x.h"
#include "encoder.h"

MODULE_SUPPORTED_DEVICE("adv717x");
MODULE_LICENSE("GPL");

EXPORT_NO_SYMBOLS;

#ifdef CONFIG_ADV717X_PIXELPORT16BIT
int pixelport_16bit[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
#else
int pixelport_16bit[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
#endif
MODULE_PARM(pixelport_16bit, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(pixelport_16bit, "Changes how the ADV717x expects its input data to be formatted. If the colours on the TV appear green, try changing this. Defaults to 1.");

#ifdef CONFIG_ADV717X_PIXELPORTPAL
int pixelport_other_pal[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
#else
int pixelport_other_pal[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
#endif
MODULE_PARM(pixelport_other_pal, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(pixelport_other_pal, "If this is set to 1, then the pixelport setting is swapped for PAL from the setting given with pixelport_16bit. Defaults to 1.");

int pixeldata_adjust_ntsc[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
MODULE_PARM(pixeldata_adjust_ntsc, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(pixeldata_adjust_ntsc, "If your red and blue colours are swapped in NTSC, try setting this to 0,1,2 or 3. Defaults to 1.");

int pixeldata_adjust_pal[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 1 };
MODULE_PARM(pixeldata_adjust_pal, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(pixeldata_adjust_pal, "If your red and blue colours are swapped in PAL, try setting this to 0,1,2 or 3. Defaults to 1.");


static int color_bars[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
MODULE_PARM(color_bars, "1-" __MODULE_STRING(EM8300_MAX) "i");
MODULE_PARM_DESC(color_bars, "If you set this to 1 a set of color bars will be displayed on your screen (used for testing if the chip is working). Defaults to 0.");

static int output_mode_nr[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = 0 };
static char *output_mode[EM8300_MAX] = { [ 0 ... EM8300_MAX-1 ] = NULL };
MODULE_PARM(output_mode, "1-" __MODULE_STRING(EM8300_MAX) "s");
MODULE_PARM_DESC(output_mode, "Specifies the output mode to use for the ADV717x video encoder. See the README-modoptions file for the list of mode names to use. Default is SVideo + composite (\"comp+svideo\").");


/* Common register offset definitions */
#define ADV717X_REG_MR0		0x00
#define ADV717X_REG_MR1		0x01
#define ADV717X_REG_TR0		0x07

/* Register offsets specific to the ADV717[56]A chips */
#define ADV7175_REG_SCFREQ0	0x02
#define ADV7175_REG_SCFREQ1	0x03
#define ADV7175_REG_SCFREQ2	0x04
#define ADV7175_REG_SCFREQ3	0x05
#define ADV7175_REG_SCPHASE	0x06
#define ADV7175_REG_CCED0	0x08
#define ADV7175_REG_CCED1	0x09
#define ADV7175_REG_CCD0	0x0a
#define ADV7175_REG_CCD1	0x0b
#define ADV7175_REG_TR1		0x0c
#define ADV7175_REG_MR2		0x0d
#define ADV7175_REG_PCR0	0x0e
#define ADV7175_REG_PCR1	0x0f
#define ADV7175_REG_PCR2	0x10
#define ADV7175_REG_PCR3	0x11
#define ADV7175_REG_MR3		0x12
#define ADV7175_REG_TTXRQ_CTRL	0x24

/* Register offsets specific to the ADV717[01] chips */
#define ADV7170_REG_MR2		0x02
#define ADV7170_REG_MR3		0x03
#define ADV7170_REG_MR4		0x04
#define ADV7170_REG_TR1		0x08
#define ADV7170_REG_SCFREQ0	0x09
#define ADV7170_REG_SCFREQ1	0x0a
#define ADV7170_REG_SCFREQ2	0x0b
#define ADV7170_REG_SCFREQ3	0x0c
#define ADV7170_REG_SCPHASE	0x0d
#define ADV7170_REG_CCED0	0x0e
#define ADV7170_REG_CCED1	0x0f
#define ADV7170_REG_CCD0	0x10
#define ADV7170_REG_CCD1	0x11
#define ADV7170_REG_PCR0	0x12
#define ADV7170_REG_PCR1	0x13
#define ADV7170_REG_PCR2	0x14
#define ADV7170_REG_PCR3	0x15
#define ADV7170_REG_TTXRQ_CTRL	0x19

static int adv717x_attach_adapter(struct i2c_adapter *adapter);
int adv717x_detach_client(struct i2c_client *client);
int adv717x_command(struct i2c_client *client, unsigned int cmd, void *arg);

typedef enum {
	MODE_COMPOSITE_SVIDEO,
	MODE_SVIDEO,
	MODE_COMPOSITE,
	MODE_COMPOSITE_PSEUDO_SVIDEO,
	MODE_PSEUDO_SVIDEO,
	MODE_COMPOSITE_OVER_SVIDEO,
	MODE_YUV,
	MODE_YUV_PROG,
	MODE_RGB,
	MODE_RGB_NOSYNC,
	MODE_RGB_PROG,
	MODE_RGB_PROG_NOSYNC,
	MODE_MAX
} OutputModes;

typedef struct {
	char const * name;
	int component;
	int yuv;
	int euroscart;
	int progressive;
	int sync_all;
	int dacA;
	int dacB;
	int dacC;
	int dacD;
} OutputModeInfo;

OutputModeInfo ModeInfo[] = {
	[ MODE_COMPOSITE_SVIDEO ] =		{ "comp+svideo" , 0, 0, 0, 0, 0, 1, 0, 0, 0 },
	[ MODE_SVIDEO ] =			{ "svideo"      , 0, 0, 0, 0, 0, 1, 1, 0, 0 },
	[ MODE_COMPOSITE ] =			{ "comp"        , 0, 0, 0, 0, 0, 1, 0, 1, 1 },
	[ MODE_COMPOSITE_PSEUDO_SVIDEO ] =	{ "comp+psvideo", 0, 0, 1, 0, 0, 1, 0, 0, 0 },
	[ MODE_PSEUDO_SVIDEO ] =		{ "psvideo"     , 0, 0, 1, 0, 0, 1, 1, 0, 0 },
	[ MODE_COMPOSITE_OVER_SVIDEO ] =	{ "composvideo" , 0, 0, 1, 0, 0, 1, 1, 1, 0 },
	[ MODE_YUV ] =				{ "yuv"         , 1, 1, 0, 0, 0, 1, 0, 0, 0 },
	[ MODE_RGB ] =				{ "rgbs"        , 1, 0, 0, 0, 1, 0, 0, 0, 0 },
	[ MODE_RGB_NOSYNC ] =			{ "rgb"         , 1, 0, 0, 0, 0, 0, 0, 0, 0 },
};

#define CHIP_ADV7175A 1
#define CHIP_ADV7170  2

struct adv717x_data_s {
	int chiptype;
	int mode;
	int bars;
	int enableoutput;
	OutputModes out_mode;
	int pp_pal;
	int pp_ntsc;
	int pd_adj_pal;
	int pd_adj_ntsc;

	unsigned char config[32];
	int configlen;
};

/* This is the driver that will be inserted */
static struct i2c_driver adv717x_driver = {
#if defined(EM8300_I2C_FORCE_NEW_API) || (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,54) && !defined(EM8300_I2C_FORCE_OLD_API))
	.owner =		THIS_MODULE,
#endif
	.name =			"ADV717X video encoder driver",
	.id =			I2C_DRIVERID_ADV717X,
	.flags =		I2C_DF_NOTIFY,
	.attach_adapter =	&adv717x_attach_adapter,
	.detach_client =	&adv717x_detach_client,
	.command =		&adv717x_command
};

int adv717x_id = 0;

static unsigned char PAL_config_7170[27] = {
	0x05,   // Mode Register 0
	0x00,   // Mode Register 1 (was: 0x07)
	0x02,   // Mode Register 2 (was: 0x48)
	0x00,   // Mode Register 3
	0x00,   // Mode Register 4
	0x00,   // Reserved
	0x00,   // Reserved
	0x0d,   // Timing Register 0
	0x77,   // Timing Register 1
	0xcb,   // Subcarrier Frequency Register 0
	0x8a,   // Subcarrier Frequency Register 1
	0x09,   // Subcarrier Frequency Register 2
	0x2a,   // Subcarrier Frequency Register 3
	0x00,   // Subcarrier Phase Register
	0x00,   // Closed Captioning Ext Register 0
	0x00,   // Closed Captioning Ext Register 1
	0x00,   // Closed Captioning Register 0
	0x00,   // Closed Captioning Register 1
	0x00,   // Pedestal Control Register 0
	0x00,   // Pedestal Control Register 1
	0x00,   // Pedestal Control Register 2
	0x00,   // Pedestal Control Register 3
	0x00,	// CGMS_WSS Reg 0
	0x00,	// CGMS_WSS Reg 1
	0x00,	// CGMS_WSS Reg 2
	0x00	// TeleText Control Register
};

static unsigned char NTSC_config_7170[27] = {
	0x10,   // Mode Register 0
	0x06,   // Mode Register 1
	0x08,   // Mode Register 2
	0x00,   // Mode Register 3
	0x00,   // Mode Register 4
	0x00,   // Reserved
	0x00,   // Reserved
	0x0d,   // Timing Register 0
	0x77,   // Timing Register 1
	0x16,   // Subcarrier Frequency Register 0
	0x7c,   // Subcarrier Frequency Register 1
	0xf0,   // Subcarrier Frequency Register 2
	0x21,   // Subcarrier Frequency Register 3
	0x00,   // Subcarrier Phase Register
	0x00,   // Closed Captioning Ext Register 0
	0x00,   // Closed Captioning Ext Register 1
	0x00,   // Closed Captioning Register 0
	0x00,   // Closed Captioning Register 1
	0x00,   // Pedestal Control Register 0
	0x00,   // Pedestal Control Register 1
	0x00,   // Pedestal Control Register 2
	0x00,   // Pedestal Control Register 3
	0x00,	// CGMS_WSS Reg 0
	0x00,	// CGMS_WSS Reg 1
	0x00,	// CGMS_WSS Reg 2
	0x00	// TeleText Control Register
};

static unsigned char PAL_M_config_7175[19] = {   //These need to be tested
	0x06,   // Mode Register 0
	0x00,   // Mode Register 1
	0xa3,   // Subcarrier Frequency Register 0
	0xef,	// Subcarrier Frequency Register 1
	0xe6,	// Subcarrier Frequency Register 2
	0x21,	// Subcarrier Frequency Register 3
	0x00,   // Subcarrier Phase Register
	0x0d,   // Timing Register 0
	0x00,   // Closed Captioning Ext Register 0
	0x00,   // Closed Captioning Ext Register 1
	0x00,   // Closed Captioning Register 0
	0x00,   // Closed Captioning Register 1
	0x70,	// Timing Register 1
	0x73,	// Mode Register 2
	0x00,   // Pedestal Control Register 0
	0x00,   // Pedestal Control Register 1
	0x00,   // Pedestal Control Register 2
	0x00,   // Pedestal Control Register 3
	0x42,   // Mode Register 3
};

static unsigned char PAL_config_7175[19] = {
	0x01,   // Mode Register 0
	0x06,   // Mode Register 1
	0xcb,   // Subcarrier Frequency Register 0
	0x8a,   // Subcarrier Frequency Register 1
	0x09,   // Subcarrier Frequency Register 2
	0x2a,   // Subcarrier Frequency Register 3
	0x00,   // Subcarrier Phase Register
	0x0d,   // Timing Register 0
	0x00,   // Closed Captioning Ext Register 0
	0x00,   // Closed Captioning Ext Register 1
	0x00,   // Closed Captioning Register 0
	0x00,   // Closed Captioning Register 1
	0x73,   // Timing Register 1
	0x08,   // Mode Register 2
	0x00,   // Pedestal Control Register 0
	0x00,   // Pedestal Control Register 1
	0x00,   // Pedestal Control Register 2
	0x00,   // Pedestal Control Register 3
	0x42,   // Mode Register 3
};

static unsigned char PAL60_config_7175[19] = {
	0x12,   // Mode Register 0
	0x0,	// Mode Register 1
	0xcb,   // Subcarrier Frequency Register 0
	0x8a,   // Subcarrier Frequency Register 1
	0x09,   // Subcarrier Frequency Register 2
	0x2a,   // Subcarrier Frequency Register 3
	0x00,   // Subcarrier Phase Register
	0x0d,   // Timing Register 0
	0x00,   // Closed Captioning Ext Register 0
	0x00,   // Closed Captioning Ext Register 1
	0x00,   // Closed Captioning Register 0
	0x00,   // Closed Captioning Register 1
	0x73,   // Timing Register 1
	0x08,   // Mode Register 2
	0x00,   // Pedestal Control Register 0
	0x00,   // Pedestal Control Register 1
	0x00,   // Pedestal Control Register 2
	0x00,   // Pedestal Control Register 3
	0x42,   // Mode Register 3
};

static unsigned char NTSC_config_7175[19] = {
	0x04,   // Mode Register 0
	0x00,   // Mode Register 1
	0x16,   // Subcarrier Frequency Register 0
	0x7c,	// Subcarrier Frequency Register 1
	0xf0,	// Subcarrier Frequency Register 2
	0x21,	// Subcarrier Frequency Register 3
	0x00,   // Subcarrier Phase Register
	0x0d,   // Timing Register 0
	0x00,   // Closed Captioning Ext Register 0
	0x00,   // Closed Captioning Ext Register 1
	0x00,   // Closed Captioning Register 0
	0x00,   // Closed Captioning Register 1
	0x77,	// Timing Register 1
	0x00,	// Mode Register 2
	0x00,   // Pedestal Control Register 0
	0x00,   // Pedestal Control Register 1
	0x00,   // Pedestal Control Register 2
	0x00,   // Pedestal Control Register 3
	0x42,   // Mode Register 3
};

#define SET_REG(f,o,v) (f) = ((f) & ~(1<<(o))) | (((v) & 1) << (o))

static int adv717x_update(struct i2c_client *client)
{
	struct adv717x_data_s *data = i2c_get_clientdata(client);
	char tmpconfig[32];
	int n, i;

	memcpy(tmpconfig, data->config, data->configlen);

	SET_REG(tmpconfig[ADV717X_REG_MR1], 7, data->bars);

	switch(data->chiptype) {
	    case CHIP_ADV7175A:
		/* ADV7175/6A component out: MR06 (register 0, bit 6) */
		SET_REG(tmpconfig[ADV717X_REG_MR0], 6,
				ModeInfo[data->out_mode].component);
		/* ADV7175/6A YUV out: MR26 (register 13, bit 6) */
		SET_REG(tmpconfig[ADV7175_REG_MR2], 6,
				ModeInfo[data->out_mode].yuv);
		/* ADV7175/6A EuroSCART: MR37 (register 18, bit 7) */
		SET_REG(tmpconfig[ADV7175_REG_MR3], 7,
			ModeInfo[data->out_mode].euroscart);
		/* ADV7175/6A RGB sync: MR05 (register 0, bit 5) */
		SET_REG(tmpconfig[ADV717X_REG_MR0], 5,
				ModeInfo[data->out_mode].sync_all);
		break;
	    case CHIP_ADV7170:
		/* ADV7170/1 component out: MR40 (register 4, bit 0) */
		SET_REG(tmpconfig[ADV7170_REG_MR4], 0,
				ModeInfo[data->out_mode].component);
		/* ADV7170/1 YUV out: MR41 (register 4, bit 1) */
		SET_REG(tmpconfig[ADV7170_REG_MR4], 1,
				ModeInfo[data->out_mode].yuv);
		/* ADV7170/1 EuroSCART: MR33 (register 3, bit 3) */
		SET_REG(tmpconfig[ADV7170_REG_MR3], 3,
			ModeInfo[data->out_mode].euroscart);
		/* ADV7170/1 RGB sync: MR42 (register 4, bit 2) */
		SET_REG(tmpconfig[ADV7170_REG_MR4], 2,
				ModeInfo[data->out_mode].sync_all);
		break;
	}
	/* ADV7170/1/5A/6A non-interlace: MR10 (register 1, bit 0) */
	SET_REG(tmpconfig[ADV717X_REG_MR1], 0,
			ModeInfo[data->out_mode].progressive);
	/* ADV7170/1/5A/6A DAC A control: MR16 (register 1, bit 6) */
	SET_REG(tmpconfig[ADV717X_REG_MR1], 6, ModeInfo[data->out_mode].dacA);
	/* ADV7170/1/5A/6A DAC B control: MR15 (register 1, bit 5) */
	SET_REG(tmpconfig[ADV717X_REG_MR1], 5, ModeInfo[data->out_mode].dacB);
	/* ADV7170/1/5A/6A DAC C control: MR13 (register 1, bit 3) */
	SET_REG(tmpconfig[ADV717X_REG_MR1], 3, ModeInfo[data->out_mode].dacC);
	/* ADV7170/1/5A/6A DAC D control: MR14 (register 1, bit 4) */
	SET_REG(tmpconfig[ADV717X_REG_MR1], 4, ModeInfo[data->out_mode].dacD);

	if (!data->enableoutput) {
		tmpconfig[ADV717X_REG_MR1] |= 0x7f;
	}

	for(i=0; i < data->configlen; i++) {
		n = i2c_smbus_write_byte_data(client, i, tmpconfig[i]);
	}

	i2c_smbus_write_byte_data(client, ADV717X_REG_TR0,
			tmpconfig[ADV717X_REG_TR0] & 0x7f);
	i2c_smbus_write_byte_data(client, ADV717X_REG_TR0,
			tmpconfig[ADV717X_REG_TR0] | 0x80);
	i2c_smbus_write_byte_data(client, ADV717X_REG_TR0,
			tmpconfig[ADV717X_REG_TR0] & 0x7f);

	return 0;
}

static int adv717x_setmode(int mode, struct i2c_client *client) {
	struct adv717x_data_s *data = i2c_get_clientdata(client);
	unsigned char *config = NULL;

	pr_debug("adv717x_setmode(%d,%p)\n", mode, client);

	switch (mode) {
	case ENCODER_MODE_PAL:
		printk(KERN_NOTICE "adv717x.o: Configuring for PAL\n");
		switch (data->chiptype) {
		case CHIP_ADV7175A:
			config = PAL_config_7175;
			data->configlen = sizeof(PAL_config_7175);
			break;
		case CHIP_ADV7170:
			config = PAL_config_7170;
			data->configlen = sizeof(PAL_config_7170);
			break;
		}
		break;
	case ENCODER_MODE_PAL_M:
		printk(KERN_NOTICE "adv717x.o: Configuring for PALM\n");
		switch (data->chiptype) {
		case CHIP_ADV7175A:
			config = PAL_config_7175;
			data->configlen = sizeof(PAL_M_config_7175);
			break;
		case CHIP_ADV7170:
			config = PAL_config_7170;
			data->configlen = sizeof(PAL_config_7170);
			break;
		}
		break;
	case ENCODER_MODE_PAL60:
		printk(KERN_NOTICE "adv717x.o: Configuring for PAL 60\n");
		switch (data->chiptype) {
		case CHIP_ADV7175A:
			config = PAL60_config_7175;
			data->configlen = sizeof(PAL60_config_7175);
			break;
		case CHIP_ADV7170:
			config = PAL_config_7170;
			data->configlen = sizeof(PAL_config_7170);
			break;
		}
		break;
	case ENCODER_MODE_NTSC:
		printk(KERN_NOTICE "adv717x.o: Configuring for NTSC\n");
		switch (data->chiptype) {
		case CHIP_ADV7175A:
			config = NTSC_config_7175;
			data->configlen = sizeof(NTSC_config_7175);
			break;
		case CHIP_ADV7170:
			config = NTSC_config_7170;
			data->configlen = sizeof(NTSC_config_7170);
			break;
		}
		break;
	default:
		return -EINVAL;
	}

	data->mode = mode;

	if (config) {
		memcpy(data->config, config, data->configlen);
	}

	switch (mode) {
	case ENCODER_MODE_PAL:
	case ENCODER_MODE_PAL_M:
	case ENCODER_MODE_PAL60:
		data->config[ADV717X_REG_TR0] =
			(data->config[ADV717X_REG_TR0] & ~0x40) | data->pp_pal;
		switch (data->chiptype) {
		case CHIP_ADV7175A:
			data->config[ADV7175_REG_TR1] = (data->config[ADV7175_REG_TR1] & ~0xC0) | data->pd_adj_pal;
			break;
		case CHIP_ADV7170:
			data->config[ADV7170_REG_TR1] = (data->config[ADV7170_REG_TR1] & ~0xC0) | data->pd_adj_pal;
			break;
		}
		break;
	case ENCODER_MODE_NTSC:
		data->config[ADV717X_REG_TR0] =
			(data->config[ADV717X_REG_TR0] & ~0x40) | data->pp_ntsc;
		switch (data->chiptype) {
		case CHIP_ADV7175A:
			data->config[ADV7175_REG_TR1] = (data->config[ADV7175_REG_TR1] & ~0xC0) | data->pd_adj_ntsc;
			break;
		case CHIP_ADV7170:
			data->config[ADV7170_REG_TR1] = (data->config[ADV7170_REG_TR1] & ~0xC0) | data->pd_adj_ntsc;
			break;
		}
		break;
	}

	return 0;
}

static int adv717x_setup(struct i2c_client *client)
{
	struct adv717x_data_s *data = i2c_get_clientdata(client);
	struct em8300_s *em = i2c_get_adapdata(client->adapter);

	memset(data->config, 0, sizeof(data->config));

	if (pixelport_16bit[em->card_nr]) {
		data->pp_ntsc = data->pp_pal = 0x40;
		if (pixelport_other_pal[em->card_nr]) {
			data->pp_pal = 0x00;
		}
	} else {
		data->pp_ntsc = data->pp_pal = 0x00;
		if (pixelport_other_pal[em->card_nr]) {
			data->pp_pal = 0x40;
		}
	}

	data->pd_adj_ntsc = (0x03 & pixeldata_adjust_ntsc[em->card_nr]) << 6;
	data->pd_adj_pal = (0x03 & pixeldata_adjust_pal[em->card_nr]) << 6;

	data->bars = color_bars[em->card_nr];
	data->enableoutput = 0;
	/* Maybe map from a string; dunno? */
	data->out_mode = output_mode_nr[em->card_nr];
	if (data->out_mode < 0 || data->out_mode >= MODE_MAX)
		data->out_mode = 0;

	adv717x_setmode(ENCODER_MODE_PAL60, client);

	adv717x_update(client);

	return 0;
}

static int adv717x_detect(struct i2c_adapter *adapter, int address)
{
	struct adv717x_data_s *data;
	struct i2c_client *new_client;
	int mr0, mr1;
	int err;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
		return 0;
	}

	if (!(new_client = kmalloc(sizeof(struct i2c_client) + sizeof(struct adv717x_data_s), GFP_KERNEL))) {
		return -ENOMEM;
	}
	memset(new_client, 0, sizeof(struct i2c_client) + sizeof(struct adv717x_data_s));
	data = (struct adv717x_data_s *) (((struct i2c_client *) new_client) + 1);
	new_client->addr = address;
	new_client->adapter = adapter;
	new_client->driver = &adv717x_driver;
	new_client->flags = 0;

	i2c_set_clientdata(new_client, data);

	i2c_smbus_write_byte_data(new_client, ADV717X_REG_MR1, 0x55);
	mr1=i2c_smbus_read_byte_data(new_client, ADV717X_REG_MR1);

	if (mr1 == 0x55) {
		mr0 = i2c_smbus_read_byte_data(new_client, ADV717X_REG_MR0);

		if (mr0 & 0x20) {
			strcpy(new_client->name, "ADV7175A chip");
			data->chiptype = CHIP_ADV7175A;
			printk(KERN_NOTICE "adv717x.o: ADV7175A chip detected\n");
		} else {
			strcpy(new_client->name, "ADV7170 chip");
			data->chiptype = CHIP_ADV7170;
			printk(KERN_NOTICE "adv717x.o: ADV7170 chip detected\n");
		}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12)
		new_client->id = adv717x_id++;
#endif

		if ((err = i2c_attach_client(new_client))) {
			kfree(new_client);
			return err;
		}

		adv717x_setup(new_client);

		EM8300_MOD_INC_USE_COUNT;

		return 0;
	}
	kfree(new_client);

	return 0;
}

static int adv717x_attach_adapter(struct i2c_adapter *adapter)
{
	adv717x_detect(adapter, 0x6a);
	adv717x_detect(adapter, 0x7a);
	adv717x_detect(adapter, 0xa);
	return 0;
}


int adv717x_detach_client(struct i2c_client *client)
{
	int err;

	if ((err = i2c_detach_client(client))) {
		printk(KERN_ERR "adv717x.o: Client deregistration failed, client not detached.\n");
		return err;
	}

	EM8300_MOD_DEC_USE_COUNT;

	kfree(client);

	return 0;
}

int adv717x_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	struct adv717x_data_s *data = i2c_get_clientdata(client);

	switch (cmd) {
	case ENCODER_CMD_SETMODE:
		adv717x_setmode((long int) arg, client);
		adv717x_update(client);
		break;
	case ENCODER_CMD_ENABLEOUTPUT:
		data->enableoutput = (long int) arg;
		adv717x_update(client);
		break;
	default:
		return -EINVAL;
		break;
	}

	return 0;
}

/* ----------------------------------------------------------------------- */


int __init adv717x_init(void)
{
	int i;
	for (i=0; i < EM8300_MAX; i++)
		if ((output_mode[i]) && (output_mode[i][0])) {
			int j;
			for (j=0; j < MODE_MAX; j++)
				if (strcmp(output_mode[i], ModeInfo[j].name) == 0) {
					output_mode_nr[i] = j;
					break;
				}
			if (j == MODE_MAX)
				printk(KERN_WARNING "adv717x.o: Unknown output mode: %s\n", output_mode[i]);
		}
	//request_module("i2c-algo-bit");
	return i2c_add_driver(&adv717x_driver);
}

void __exit adv717x_cleanup(void)
{
	i2c_del_driver(&adv717x_driver);
}

module_init(adv717x_init);
module_exit(adv717x_cleanup);
