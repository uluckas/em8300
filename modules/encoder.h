#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <linux/em8300.h>

#define ENCODER_MODE_NTSC	1
#define ENCODER_MODE_NTSC60	2
#define	ENCODER_MODE_PAL_M	3
#define ENCODER_MODE_PALM60	4
#define	ENCODER_MODE_PAL	5
#define ENCODER_MODE_PAL60	6
#define ENCODER_MODE_PALNC	7

/** cch
#define ENCODER_MODE_PAL	1
#define ENCODER_MODE_PAL60	2
#define ENCODER_MODE_NTSC	3
#define ENCODER_MODE_PAL_M	4
**/

#define ENCODER_CMD_SETMODE      1
#define ENCODER_CMD_ENABLEOUTPUT 2

#endif

