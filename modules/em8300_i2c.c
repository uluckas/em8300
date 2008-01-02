#define __NO_VERSION__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include "em8300_compat24.h"
#include "em8300_reg.h"
#include <linux/em8300.h>

#include "adv717x.h"
#include "bt865.h"
#include "encoder.h"
//#include <linux/sensors.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#define sysfs_create_link(kobj, target, name) do {} while (0)
#define sysfs_remove_link(kobj, name) do {} while (0)
#else
#include <linux/sysfs.h>
#endif

#define I2C_HW_B_EM8300 0xa

struct private_data_s {
	int clk;
	int data;
	struct em8300_s *em;
};

/* ----------------------------------------------------------------------- */
/* I2C bitbanger functions						   */
/* ----------------------------------------------------------------------- */

/* software I2C functions */

static void em8300_setscl(void *data,int state)
{
	struct private_data_s *p = (struct private_data_s *) data;
	struct em8300_s *em = p->em;
	int sel = p->clk << 8;

	writel(sel | p->clk, &em->mem[em->i2c_oe_reg]);
	writel(sel | (state ? p->clk : 0), &em->mem[em->i2c_pin_reg]);
}

static void em8300_setsda(void *data, int state)
{
	struct private_data_s *p = (struct private_data_s *) data;
	struct em8300_s *em = p->em;
	int sel = p->data << 8;

	writel(sel | p->data, &em->mem[em->i2c_oe_reg]);
	writel(sel | (state ? p->data : 0), &em->mem[em->i2c_pin_reg]);
}

static int em8300_getscl(void *data)
{
	struct private_data_s *p = (struct private_data_s *)data;
	struct em8300_s *em = p->em;

	return readl(&em->mem[em->i2c_pin_reg]) & (p->clk << 8);
}

static int em8300_getsda(void *data)
{
	struct private_data_s *p = (struct private_data_s *)data;
	struct em8300_s *em = p->em;

	return readl(&em->mem[em->i2c_pin_reg]) & (p->data << 8);
}

static int em8300_i2c_lock_client(struct i2c_client *client)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,54)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	if (!try_module_get(client->driver->driver.owner)) {
#else
	if (!try_module_get(client->driver->owner)) {
#endif
		printk(KERN_ERR "em8300_i2c: Unable to lock client module\n");
		return -ENODEV;
	}
#endif
	return 0;
}

static int em8300_i2c_reg(struct i2c_client *client)
{
	struct em8300_s *em = i2c_get_adapdata(client->adapter);

	switch (client->driver->id) {
	case I2C_DRIVERID_ADV717X:
		if (em8300_i2c_lock_client(client)) {
			return -ENODEV;
		}
		if (!strncmp(client->name, "ADV7175", 7)) {
			em->encoder_type = ENCODER_ADV7175;
		}
		if (!strncmp(client->name, "ADV7170", 7)) {
			em->encoder_type = ENCODER_ADV7170;
		}
		em->encoder = client;
		sysfs_create_link(&em->dev->dev.kobj, &client->dev.kobj, "encoder");
		client->driver->command(client, ENCODER_CMD_ENABLEOUTPUT, (void *)0);
		do {
			struct getconfig_s data;
			struct setparam_s param;
			data.card_nr = em->card_nr;
			if (client->driver->command(client,
						    ENCODER_CMD_GETCONFIG,
						    (void *) &data) != 0) {
				printk("ENCODER_CMD_GETCONFIG failed\n");
				break;
			}
			param.param = ENCODER_PARAM_COLORBARS;
			param.modes = (uint32_t)-1;
			param.val = data.config[4]?1:0;
			client->driver->command(client,
						ENCODER_CMD_SETPARAM,
						&param);
			param.param = ENCODER_PARAM_OUTPUTMODE;
			param.val = data.config[5];
			client->driver->command(client,
						ENCODER_CMD_SETPARAM,
						&param);
			param.param = ENCODER_PARAM_PPORT;
			param.modes = NTSC_MODES_MASK;
			param.val = data.config[0]?1:0;
			client->driver->command(client,
						ENCODER_CMD_SETPARAM,
						&param);
			param.modes = PAL_MODES_MASK;
			param.val = data.config[1]
				? (data.config[0]?0:1)
				: (data.config[0]?1:0);
			client->driver->command(client,
						ENCODER_CMD_SETPARAM,
						&param);
			param.param = ENCODER_PARAM_PDADJ;
			param.modes = NTSC_MODES_MASK;
			param.val = data.config[2];
			client->driver->command(client,
						ENCODER_CMD_SETPARAM,
						&param);
			param.modes = PAL_MODES_MASK;
			param.val = data.config[3];
			client->driver->command(client,
						ENCODER_CMD_SETPARAM,
						&param);
		} while (0);
		break;
	case I2C_DRIVERID_BT865:
		if (em8300_i2c_lock_client(client)) {
			return -ENODEV;
		}
		em->encoder_type = ENCODER_BT865;
		em->encoder = client;
		sysfs_create_link(&em->dev->dev.kobj, &client->dev.kobj, "encoder");
		break;
	case I2C_DRIVERID_EEPROM:
		sysfs_create_link(&em->dev->dev.kobj, &client->dev.kobj, "eeprom");
		break;
	default:
		printk(KERN_ERR "em8300_i2c: unknown client id\n");
		return -ENODEV;
	}

	return 0;
}

static int em8300_i2c_unreg(struct i2c_client *client)
{
	struct em8300_s *em = i2c_get_adapdata(client->adapter);

	switch (client->driver->id) {
	case I2C_DRIVERID_ADV717X:
	case I2C_DRIVERID_BT865:
		sysfs_remove_link(&em->dev->dev.kobj, "encoder");
		em->encoder = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,54)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		module_put(client->driver->driver.owner);
#else
		module_put(client->driver->owner);
#endif
#endif
		break;
	case I2C_DRIVERID_EEPROM:
		sysfs_remove_link(&em->dev->dev.kobj, "eeprom");
		break;
	}

	return 0;
}

/* ----------------------------------------------------------------------- */
/* I2C functions							   */
/* ----------------------------------------------------------------------- */
int em8300_i2c_init1(struct em8300_s *em)
{
	int ret;
	struct private_data_s *pdata;

	//request_module("i2c-algo-bit");

	switch (em->chip_revision) {
	case 2:
		em->i2c_oe_reg = EM8300_I2C_OE;
		em->i2c_pin_reg = EM8300_I2C_PIN;
		break;
	case 1:
		em->i2c_oe_reg = 0x1f4f;
		em->i2c_pin_reg = EM8300_I2C_OE;
		break;
	}

	/*
	  Reset devices on I2C bus
	*/
	writel(0x3f3f, &em->mem[em->i2c_pin_reg]);
	writel(0x3b3b, &em->mem[em->i2c_oe_reg]);
	writel(0x0100, &em->mem[em->i2c_pin_reg]);
	writel(0x0101, &em->mem[em->i2c_pin_reg]);
	writel(0x0808, &em->mem[em->i2c_pin_reg]);

	/*
	  Setup info structure for bus 2
	*/

	em->i2c_data_2.setsda = &em8300_setsda;
	em->i2c_data_2.setscl = &em8300_setscl;
	em->i2c_data_2.getsda = &em8300_getsda;
	em->i2c_data_2.getscl = &em8300_getscl;
	em->i2c_data_2.udelay = 10;
	em->i2c_data_2.timeout = 100;

	pdata = kmalloc(sizeof(struct private_data_s), GFP_KERNEL);
	pdata->clk = 0x4;
	pdata->data = 0x8;
	pdata->em = em;

	em->i2c_data_2.data = pdata;

	strcpy(em->i2c_ops_2.name, "EM8300 I2C bus 2");
	em->i2c_ops_2.id = I2C_HW_B_EM8300;
	em->i2c_ops_2.algo = NULL;
	em->i2c_ops_2.algo_data = &em->i2c_data_2;
	em->i2c_ops_2.client_register = em8300_i2c_reg;
	em->i2c_ops_2.client_unregister = em8300_i2c_unreg;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	em->i2c_ops_2.dev.parent = &em->dev->dev;
#endif

	i2c_set_adapdata(&em->i2c_ops_2, (void *)em);

	ret = i2c_bit_add_bus(&em->i2c_ops_2);
	return ret;
}

int em8300_i2c_init2(struct em8300_s *em)
{
	int ret;
	struct private_data_s *pdata;

	/*
	  Setup info structure for bus 1
	*/

	em->i2c_data_1.setsda = &em8300_setsda;
	em->i2c_data_1.setscl = &em8300_setscl;
	em->i2c_data_1.getsda = &em8300_getsda;
	em->i2c_data_1.getscl = &em8300_getscl;
	em->i2c_data_1.udelay = 10;
	em->i2c_data_1.timeout = 100;

	pdata = kmalloc(sizeof(struct private_data_s),GFP_KERNEL);
	pdata->clk = 0x10;
	pdata->data = 0x8;
	pdata->em = em;

	em->i2c_data_1.data = pdata;

	strcpy(em->i2c_ops_1.name, "EM8300 I2C bus 1");
	em->i2c_ops_1.id = I2C_HW_B_EM8300;
	em->i2c_ops_1.algo = NULL;
	em->i2c_ops_1.algo_data = &em->i2c_data_1;
	em->i2c_ops_1.client_register = em8300_i2c_reg;
	em->i2c_ops_1.client_unregister = em8300_i2c_unreg;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	em->i2c_ops_1.dev.parent = &em->dev->dev;
#endif

	i2c_set_adapdata(&em->i2c_ops_1, (void *)em);

	ret = i2c_bit_add_bus(&em->i2c_ops_1);
	return ret;
}

void em8300_i2c_exit(struct em8300_s *em)
{
	/* unregister i2c_bus */
	kfree(em->i2c_data_1.data);
	kfree(em->i2c_data_2.data);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	i2c_del_adapter(&em->i2c_ops_1);
	i2c_del_adapter(&em->i2c_ops_2);
#else
	i2c_bit_del_bus(&em->i2c_ops_1);
	i2c_bit_del_bus(&em->i2c_ops_2);
#endif
}

void em8300_clockgen_write(struct em8300_s *em, int abyte)
{
	int i;

	writel(0x808, &em->mem[em->i2c_pin_reg]);
	for (i=0; i < 8; i++) {
		writel(0x2000, &em->mem[em->i2c_pin_reg]);
		writel(0x800 | ((abyte & 1) ? 8 : 0), &em->mem[em->i2c_pin_reg]);
		writel(0x2020, &em->mem[em->i2c_pin_reg]);
		abyte >>= 1;
	}

	writel(0x200, &em->mem[em->i2c_pin_reg]);
	udelay(10);
	writel(0x202, &em->mem[em->i2c_pin_reg]);
}

static void I2C_clk(struct em8300_s *em, int level)
{
	writel(0x1000 | (level ? 0x10 : 0), &em->mem[em->i2c_pin_reg]);
	udelay(10);
}

static void I2C_data(struct em8300_s *em, int level)
{
	writel(0x800 | (level ? 0x8 : 0), &em->mem[em->i2c_pin_reg]);
	udelay(10);
}

static void I2C_drivedata(struct em8300_s *em, int level)
{
	writel(0x800 | (level ? 0x8 : 0), &em->mem[em->i2c_oe_reg]);
	udelay(10);
}

#define I2C_read_data ((readl(&em->mem[em->i2c_pin_reg]) & 0x800) ? 1 : 0)

static void I2C_out(struct em8300_s *em, int data, int bits)
{
	int i;
	for (i = bits - 1; i >= 0; i--) {
		I2C_data(em, data & (1 << i));
		I2C_clk(em, 1);
		I2C_clk(em, 0);
	}
}

static int I2C_in(struct em8300_s *em, int bits)
{
	int i, data = 0;

	for(i = bits - 1; i >= 0; i--) {
		data |= I2C_read_data << i;
		I2C_clk(em, 0);
		I2C_clk(em, 1);
	}
	return data;
}

static void sub_23660(struct em8300_s *em, int arg1, int arg2)
{
	I2C_clk(em, 0);
	I2C_out(em, arg1, 8);
	I2C_data(em, arg2);
	I2C_clk(em, 1);
}


static void sub_236f0 (struct em8300_s *em,int arg1, int arg2, int arg3)
{
	I2C_clk(em, 1);
	I2C_data(em, 1);
	I2C_clk(em, 0);
	I2C_data(em, 1);
	I2C_clk(em, 1);
	I2C_clk(em, 0);

	sub_23660(em, 1, arg2);

	sub_23660(em, arg1, arg3);
}

void em9010_write(struct em8300_s *em, int reg, int data)
{
	sub_236f0(em, reg, 1, 0);
	sub_23660(em, data, 1);
}

int em9010_read(struct em8300_s *em, int reg)
{
	int val;

	sub_236f0(em, reg, 0, 0);
	I2C_drivedata(em, 0);
	val = I2C_in(em, 8);
	I2C_drivedata(em, 1);
	I2C_clk(em, 0);
	I2C_data(em, 1);
	I2C_clk(em, 1);

	return val;
}

/* loc_2A5d8 in cl.asm
   call dword ptr [exx+0x14]
*/
int em9010_read16(struct em8300_s *em, int reg)
{
	if (reg > 128) {
		em9010_write(em, 3, 0);
		em9010_write(em, 4, reg);
	} else {
		em9010_write(em, 4, 0);
		em9010_write(em, 3, reg);
	}

	return em9010_read(em, 2) | (em9010_read(em, 1) << 8);
}

/* loc_2A558 in cl.asm
   call dword ptr [exx+0x10]
*/
void em9010_write16(struct em8300_s *em, int reg, int value)
{
	if (reg > 128) {
		em9010_write(em, 3, 0);
		em9010_write(em, 4, reg);
	} else {
		em9010_write(em, 4, 0);
		em9010_write(em, 3, reg);
	}
	em9010_write(em, 2, value & 0xff);
	em9010_write(em, 1, value >> 8);
}

