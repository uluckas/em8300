#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <linux/em8300.h>

#define ENCODER_MODE_NTSC	 1
#define ENCODER_MODE_NTSC60	 2
#define ENCODER_MODE_PAL_M	 3
#define ENCODER_MODE_PALM60	 4
#define ENCODER_MODE_PAL	 5
#define ENCODER_MODE_PAL60	 6
#define ENCODER_MODE_PALNC	 7

#define PAL_MODES_MASK ((uint32_t)((1u<<(ENCODER_MODE_PAL)) \
                                  |(1u<<(ENCODER_MODE_PAL_M)) \
                                  |(1u<<(ENCODER_MODE_PAL60))))
#define NTSC_MODES_MASK ((uint32_t)((1u<<(ENCODER_MODE_NTSC))))

#define ENCODER_PARAM_COLORBARS  1
#define ENCODER_PARAM_OUTPUTMODE 2
#define ENCODER_PARAM_PPORT      3 /* ADV717x specific */
#define ENCODER_PARAM_PDADJ      4 /* ADV717x specific */

#define ENCODER_CMD_SETMODE      1
#define ENCODER_CMD_ENABLEOUTPUT 2

struct setparam_s {
	int param;
	uint32_t modes;
	int val;
};
#define ENCODER_CMD_SETPARAM     3

struct getconfig_s {
	unsigned int card_nr;
	int config[6];
};
#define ENCODER_CMD_GETCONFIG    0xDEADBEEF

#endif

