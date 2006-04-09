/* $Id$
 *
 * em8300_sysfs.c -- interface to the sysfs filesystem
 * Copyright (C) 2004 Eric Donohue <epd3j@hotmail.com>
 * Copyright (C) 2004,2006 Nicolas Boullis <nboullis@debian.org>
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

#include "em8300_sysfs.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,46)

#include <linux/device.h>
#include <linux/pci.h>

extern struct pci_driver em8300_driver;

static ssize_t show_version(struct device_driver *dd, char *buf)
{
	return sprintf(buf, "%s\n", EM8300_VERSION);
}

static DRIVER_ATTR(version, S_IRUGO, show_version, NULL);

static void em8300_sysfs_postregister_driver(void)
{
	driver_create_file(&em8300_driver.driver, &driver_attr_version);
}

static void em8300_sysfs_register_card(struct em8300_s *em)
{
}

static void em8300_sysfs_unregister_card(struct em8300_s *em)
{
}

static void em8300_sysfs_preunregister_driver(void)
{
	driver_remove_file(&em8300_driver.driver, &driver_attr_version);
}

struct em8300_registrar_s em8300_sysfs_registrar =
{
	.register_driver      = NULL,
	.postregister_driver  = &em8300_sysfs_postregister_driver,
	.register_card        = &em8300_sysfs_register_card,
	.enable_card          = NULL,
	.disable_card         = NULL,
	.unregister_card      = &em8300_sysfs_unregister_card,
	.preunregister_driver = &em8300_sysfs_preunregister_driver,
	.unregister_driver    = NULL,
	.audio_interrupt      = NULL,
	.video_interrupt      = NULL,
	.vbl_interrupt        = NULL,
};

#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,46) */

struct em8300_registrar_s em8300_sysfs_registrar =
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

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,46) */
