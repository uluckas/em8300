/*
	em8300.c - EM8300 MPEG-2 decoder device driver

	Copyright (C) 2000 Henrik Johansson <henrikjo@post.utfors.se>

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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <linux/config.h>
#include "em8300_devfs.h"
#include <linux/devfs_fs_kernel.h>

#ifdef CONFIG_DEVFS_FS

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,70)
devfs_handle_t em8300_handle[EM8300_MAX*4];
#endif

extern struct file_operations em8300_fops;

static void em8300_devfs_register_card(struct em8300_s *em)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,70)
	char devname[64];
	sprintf(devname, "%s-%d", EM8300_LOGNAME, em->card_nr );
	em8300_handle[em->card_nr * 4] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, EM8300_MAJOR,
							em->card_nr * 4, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
	sprintf(devname, "%s_mv-%d", EM8300_LOGNAME, em->card_nr );
	em8300_handle[(em->card_nr * 4) + 1] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, EM8300_MAJOR,
							      (em->card_nr * 4) + 1, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
	sprintf(devname, "%s_ma-%d", EM8300_LOGNAME, em->card_nr );
	em8300_handle[(em->card_nr * 4) + 2] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, EM8300_MAJOR,
							      (em->card_nr * 4) + 2, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
	sprintf(devname, "%s_sp-%d", EM8300_LOGNAME, em->card_nr );
	em8300_handle[(em->card_nr * 4) + 3] = devfs_register(NULL, devname, DEVFS_FL_DEFAULT, EM8300_MAJOR,
							      (em->card_nr * 4) + 3, S_IFCHR | S_IRUGO | S_IWUGO, &em8300_fops, NULL);
#else
	devfs_mk_cdev(MKDEV(EM8300_MAJOR, em->card_nr * 4),
		      S_IFCHR | S_IRUGO | S_IWUGO,
		      "%s-%d", EM8300_LOGNAME, em->card_nr);
	devfs_mk_cdev(MKDEV(EM8300_MAJOR, (em->card_nr * 4) + 1),
		      S_IFCHR | S_IRUGO | S_IWUGO,
		      "%s_mv-%d", EM8300_LOGNAME, em->card_nr);
	devfs_mk_cdev(MKDEV(EM8300_MAJOR, (em->card_nr * 4) + 2),
		      S_IFCHR | S_IRUGO | S_IWUGO,
		      "%s_ma-%d", EM8300_LOGNAME, em->card_nr);
	devfs_mk_cdev(MKDEV(EM8300_MAJOR, (em->card_nr * 4) + 3),
		      S_IFCHR | S_IRUGO | S_IWUGO,
		      "%s_sp-%d", EM8300_LOGNAME, em->card_nr);
#endif
}

static void em8300_devfs_unregister_card(struct em8300_s *em)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,69)
	devfs_unregister(em8300_handle[em->card_nr * 4]);
	devfs_unregister(em8300_handle[(em->card_nr * 4) + 1]);
	devfs_unregister(em8300_handle[(em->card_nr * 4) + 2]);
	devfs_unregister(em8300_handle[(em->card_nr * 4) + 3]);
#else
	devfs_remove("%s-%d", EM8300_LOGNAME, em->card_nr);
	devfs_remove("%s_mv-%d", EM8300_LOGNAME, em->card_nr);
	devfs_remove("%s_ma-%d", EM8300_LOGNAME, em->card_nr);
	devfs_remove("%s_sp-%d", EM8300_LOGNAME, em->card_nr);
#endif
}

struct em8300_registrar_s em8300_devfs_registrar =
{
	.register_driver   = NULL,
	.register_card     = &em8300_devfs_register_card,
	.enable_card       = NULL,
	.disable_card      = NULL,
	.unregister_card   = &em8300_devfs_unregister_card,
	.unregister_driver = NULL,
};

#else /* CONFIG_DEVFS_FS */

struct em8300_registrar_s em8300_devfs_registrar =
{
	.register_driver   = NULL,
	.register_card     = NULL,
	.enable_card       = NULL,
	.disable_card      = NULL,
	.unregister_card   = NULL,
	.unregister_driver = NULL,
};

#endif /* CONFIG_DEVFS_FS */
