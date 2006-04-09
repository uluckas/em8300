/* $Id$
 *
 * em8300_udev.c -- interface for udev
 * Copyright (C) 2006 Nicolas Boullis <nboullis@debian.org>
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "em8300_udev.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,2)

#include <linux/device.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
#define class class_simple
#define class_create class_simple_create
#define class_device_create class_simple_device_add
#define class_device_destroy(cls, dev) class_simple_device_remove(dev)
#define class_destroy class_simple_destroy
#endif

struct class *em8300_class;

static void em8300_udev_register_driver(void)
{
	em8300_class = class_create(THIS_MODULE, "em8300");
}

static void em8300_udev_register_card(struct em8300_s *em)
{
	em->classdev = class_device_create(em8300_class,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
					   NULL,
#endif
					   MKDEV(major, em->card_nr * 4 + 0),
					   &em->dev->dev,
					   "%s-%d",
					   EM8300_LOGNAME, em->card_nr);
}

static void em8300_udev_enable_card(struct em8300_s *em)
{
	em->classdev_mv = class_device_create(em8300_class,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
					      em->classdev,
#endif
					      MKDEV(major, em->card_nr * 4 + 1),
					      &em->dev->dev,
					      "%s_mv-%d",
					      EM8300_LOGNAME, em->card_nr);
	em->classdev_ma = class_device_create(em8300_class,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
					      em->classdev,
#endif
					      MKDEV(major, em->card_nr * 4 + 2),
					      &em->dev->dev,
					      "%s_ma-%d",
					      EM8300_LOGNAME, em->card_nr);
	em->classdev_sp = class_device_create(em8300_class,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
					      em->classdev,
#endif
					      MKDEV(major, em->card_nr * 4 + 3),
					      &em->dev->dev,
					      "%s_sp-%d",
					      EM8300_LOGNAME, em->card_nr);
}

static void em8300_udev_disable_card(struct em8300_s *em)
{
	class_device_destroy(em8300_class, MKDEV(major, em->card_nr * 4 + 1));
	class_device_destroy(em8300_class, MKDEV(major, em->card_nr * 4 + 2));
	class_device_destroy(em8300_class, MKDEV(major, em->card_nr * 4 + 3));
}

static void em8300_udev_unregister_card(struct em8300_s *em)
{
	class_device_destroy(em8300_class, MKDEV(major, em->card_nr * 4 + 0));
}

static void em8300_udev_unregister_driver(void)
{
	class_destroy(em8300_class);
}

struct em8300_registrar_s em8300_udev_registrar =
{
	.register_driver      = &em8300_udev_register_driver,
	.postregister_driver  = NULL,
	.register_card        = &em8300_udev_register_card,
	.enable_card          = &em8300_udev_enable_card,
	.disable_card         = &em8300_udev_disable_card,
	.unregister_card      = &em8300_udev_unregister_card,
	.preunregister_driver = NULL,
	.unregister_driver    = &em8300_udev_unregister_driver,
	.audio_interrupt      = NULL,
	.video_interrupt      = NULL,
	.vbl_interrupt        = NULL,
};

#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,2) */

struct em8300_registrar_s em8300_udev_registrar =
{
	.register_driver      = NULL,
	.postregister_driver  = NULL,
	.register_card        = NULL,
	.enable_card          = NULL,
	.disable_card         = NULL,
	.unregister_card      = NULL,
	.preunregister_driver = NULL,
	.unregister_driver    = NULL,
	.audio_interrupt      = NULL,
	.video_interrupt      = NULL,
	.vbl_interrupt        = NULL,
};

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,2) */
