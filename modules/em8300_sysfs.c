/* $Id$
 *
 * em8300_sysfs.c -- interface to the sysfs filesystem
 * Copyright (C) 2004 Eric Donohue <epd3j@hotmail.com>
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

#include "em8300_sysfs.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,46)

#include <linux/pci.h>
#include <linux/device.h>

#ifndef EM8300_SYSFS_DIR
#define EM8300_SYSFS_DIR "media"
#endif

static void em8300_class_release(struct class_device *cd)
{
	printk (KERN_NOTICE "em8300_class_release()\n");
}

static struct class em8300_class = {
	.name = EM8300_SYSFS_DIR,
	.release = em8300_class_release
};

static ssize_t show_version(struct class_device *cd, char *buf)
{
	sprintf(buf, "%s\n", EM8300_VERSION);
	return strlen(buf) + 1;
}

static CLASS_DEVICE_ATTR(version, S_IRUGO, show_version, NULL);

static ssize_t show_devnum(struct class_device *cd, char *buf)
{
	char name[64];
	struct em8300_s *em = container_of(cd, struct em8300_s, classdev);
	sprintf(name, "%s-%d", EM8300_LOGNAME, em->card_nr);
	if(!strcmp(cd->class_id, name)) {
		dev_t dev = MKDEV(major, em->card_nr * 4 + 0);
		return print_dev_t(buf, dev);
	}
	sprintf(name, "%s_mv-%d", EM8300_LOGNAME, em->card_nr);
	if(!strcmp(cd->class_id, name)) {
		dev_t dev = MKDEV(major, em->card_nr * 4 + 1);
		return print_dev_t(buf, dev);
	}
	sprintf(name, "%s_ma-%d", EM8300_LOGNAME, em->card_nr);
	if(!strcmp(cd->class_id, name)) {
		dev_t dev = MKDEV(major, em->card_nr * 4 + 2);
		return print_dev_t(buf, dev);
	}
	sprintf(name, "%s_sp-%d", EM8300_LOGNAME, em->card_nr);
	if(!strcmp(cd->class_id, name)) {
		dev_t dev = MKDEV(major, em->card_nr * 4 + 3);
		return print_dev_t(buf, dev);
	}
	return -ENODEV;
}

static CLASS_DEVICE_ATTR(dev, S_IRUGO, show_devnum, NULL);

static struct class_device_attribute *em8300_attrs[] = {
	&class_device_attr_version,
	&class_device_attr_dev,
	NULL
};

static void em8300_sysfs_register_driver(void)
{
	class_register(&em8300_class);
}

static void em8300_sysfs_register_card(struct em8300_s *em)
{
	char name[64] = "";
	int i;

	em->classdev.class = &em8300_class;

	em->classdev.dev = &em->dev->dev;

	sprintf(name,"%s-%d", EM8300_LOGNAME, em->card_nr);
	strcpy(em->classdev.class_id, name);

	if(class_device_register(&em->classdev))
		printk(KERN_ERR "Unable to register em8300 class device\n");

	for(i = 0; em8300_attrs[i]; i++)
		class_device_create_file(&(em->classdev), em8300_attrs[i]);
}

static void em8300_sysfs_enable_card(struct em8300_s *em)
{
	char name[64] = "";
	int i;

	em->classdev_mv.class = &em8300_class;
	em->classdev_ma.class = &em8300_class;
	em->classdev_sp.class = &em8300_class;

	em->classdev_mv.dev = &em->dev->dev;
	em->classdev_ma.dev = &em->dev->dev;
	em->classdev_sp.dev = &em->dev->dev;

	sprintf(name,"%s_mv-%d", EM8300_LOGNAME, em->card_nr);
	strcpy(em->classdev_mv.class_id, name);
	sprintf(name,"%s_ma-%d", EM8300_LOGNAME, em->card_nr);
	strcpy(em->classdev_ma.class_id, name);
	sprintf(name,"%s_sp-%d", EM8300_LOGNAME, em->card_nr);
	strcpy(em->classdev_sp.class_id, name);

	if(class_device_register(&em->classdev_mv))
		printk(KERN_ERR "Unable to register em8300_mv class device\n");
	if(class_device_register(&em->classdev_ma))
		printk(KERN_ERR "Unable to register em8300_ma class device\n");
	if(class_device_register(&em->classdev_sp))
		printk(KERN_ERR "Unable to register em8300_sp class device\n");

	for(i = 0; em8300_attrs[i]; i++) {
		class_device_create_file(&(em->classdev_mv), em8300_attrs[i]);
		class_device_create_file(&(em->classdev_ma), em8300_attrs[i]);
		class_device_create_file(&(em->classdev_sp), em8300_attrs[i]);
	}
}

static void em8300_sysfs_disable_card(struct em8300_s *em)
{
	class_device_unregister(&(em->classdev_mv));
	class_device_unregister(&(em->classdev_ma));
	class_device_unregister(&(em->classdev_sp));
}

static void em8300_sysfs_unregister_card(struct em8300_s *em)
{
	class_device_unregister(&(em->classdev));
}

static void em8300_sysfs_unregister_driver(void)
{
	class_unregister(&em8300_class);
}

struct em8300_registrar_s em8300_sysfs_registrar =
{
	.register_driver   = &em8300_sysfs_register_driver,
	.register_card     = &em8300_sysfs_register_card,
	.enable_card       = &em8300_sysfs_enable_card,
	.disable_card      = &em8300_sysfs_disable_card,
	.unregister_card   = &em8300_sysfs_unregister_card,
	.unregister_driver = &em8300_sysfs_unregister_driver,
};

#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,46) */

struct em8300_registrar_s em8300_sysfs_registrar =
{
	.register_driver   = NULL,
	.register_card     = NULL,
	.enable_card       = NULL,
	.disable_card      = NULL,
	.unregister_card   = NULL,
	.unregister_driver = NULL,
};

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,46) */
