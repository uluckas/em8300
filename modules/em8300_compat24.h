/* $Id$
 *
 * em8300_compat24.h -- compatibility layer for 2.4 and some 2.5 kernels
 * Copyright (C) 2004 Andreas Schultz <aschultz@warp10.net>
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

#ifndef _EM8300_COMPAT24_H_
#define _EM8300_COMPAT24_H_

/* Interrupt handler backwards compatibility stuff */
#ifndef IRQ_NONE
#define IRQ_NONE
#define IRQ_HANDLED
typedef void irqreturn_t;
#endif

/* i2c stuff */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,67)
static inline void *i2c_get_clientdata(struct i2c_client *dev)
{
	return dev->data;
}

static inline void i2c_set_clientdata(struct i2c_client *dev, void *data)
{
	dev->data = data;
}

static inline void *i2c_get_adapdata(struct i2c_adapter *dev)
{
	return dev->data;
}

static inline void i2c_set_adapdata(struct i2c_adapter *dev, void *data)
{
	dev->data = data;
}
#endif

/* modules */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,48)
#define EM8300_MOD_INC_USE_COUNT MOD_INC_USE_COUNT
#define EM8300_MOD_DEC_USE_COUNT MOD_DEC_USE_COUNT
#else
#define EM8300_MOD_INC_USE_COUNT do { } while(0)
#define EM8300_MOD_DEC_USE_COUNT do { } while(0)
#endif

#if !defined(MODULE_LICENSE)
#define MODULE_LICENSE(_license)
#endif

#if !defined(EXPORT_NO_SYMBOLS)
#define EXPORT_NO_SYMBOLS
#endif

/* EM8300_IMINOR */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,2) || LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#define EM8300_IMINOR(inode) (MINOR((inode)->i_rdev))
#else
#define EM8300_IMINOR(inode) (minor((inode)->i_rdev))
#endif

#endif /* _EM8300_COMPAT24_H_ */
