#define __NO_VERSION__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/malloc.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/i2c-algo-bit.h>

#include "em8300_reg.h"

#include "em8300.h"

#include "adv717x.h"
//#include <linux/sensors.h>

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

static
void em8300_setscl(void *data,int state)
{
    struct private_data_s *p = (struct private_data_s *)data;
    int sel = p->clk << 8;

    p->em->mem[p->em->i2c_oe_reg] = sel | p->clk;
    p->em->mem[p->em->i2c_pin_reg] = sel | (state ? p->clk : 0);
}

static
void em8300_setsda(void *data,int state)
{
    struct private_data_s *p = (struct private_data_s *)data;
    struct em8300_s *em = p->em;
    int sel = p->data << 8;

    em->mem[em->i2c_oe_reg] = sel | p->data;
    em->mem[em->i2c_pin_reg] = sel | (state ? p->data : 0);
}

static
int em8300_getscl(void *data)
{
    struct private_data_s *p = (struct private_data_s *)data;
    struct em8300_s *em = p->em;

    return em->mem[em->i2c_pin_reg] & (p->clk << 8);
}

static
int em8300_getsda(void *data)
{
    struct private_data_s *p = (struct private_data_s *)data;
    struct em8300_s *em = p->em;

    return em->mem[em->i2c_pin_reg] & (p->data << 8);
}


static int em8300_i2c_reg(struct i2c_client *client)
{
    struct em8300_s *em = client->adapter->data;

    switch(client->driver->id) {
    case I2C_DRIVERID_ADV717X:
	if(!strncmp(client->name, "ADV7175", 7))
	    em->encoder_type = ENCODER_ADV7175;
	if(!strncmp(client->name, "ADV7170", 7))
	    em->encoder_type = ENCODER_ADV7170;
	em->encoder = client;
	break;
    case I2C_DRIVERID_AT24Cxx:
	em->eeprom = client;
	break;
    }
    return 0;
}

static int em8300_i2c_unreg(struct i2c_client *client)
{
    struct em8300_s *em = client->adapter->data;

    switch(client->driver->id) {
    case I2C_DRIVERID_ADV717X:
	em->encoder = NULL;
	break;
/*
    case I2C_DRIVERID_EEPROM:
	em->eeprom = NULL;
	break;
*/
    }
    return 0;
}

static void em8300_i2c_inc(struct i2c_adapter *adapter)
{	
}

static void em8300_i2c_dec(struct i2c_adapter *adapter)
{	
}

/* ----------------------------------------------------------------------- */
/* I2C functions							   */
/* ----------------------------------------------------------------------- */
int em8300_i2c_init(struct em8300_s *em)
{
    int ret;
    struct private_data_s *pdata;

    switch(em->chip_revision) {
    case 2:
	em->i2c_oe_reg = 0x1f4e;
	em->i2c_pin_reg = 0x1f4d;
	break;
    case 1:
	em->i2c_oe_reg = 0x1f4f;
	em->i2c_pin_reg = 0x1f4e;
	break;
    }
    
    /*
      Reset devices on I2C bus
    */
    em->mem[em->i2c_pin_reg] = 0x3f3f;
    em->mem[em->i2c_oe_reg] = 0x3b3b;
    em->mem[em->i2c_pin_reg] = 0x0100;
    em->mem[em->i2c_pin_reg] = 0x0101;
    em->mem[em->i2c_pin_reg] = 0x0808;
    
    /*
      Setup info structure for bus 1
    */
    
    em->i2c_data_1.setsda = &em8300_setsda;
    em->i2c_data_1.setscl = &em8300_setscl;
    em->i2c_data_1.getsda = &em8300_getsda;
    em->i2c_data_1.getscl = &em8300_getscl;
    em->i2c_data_1.mdelay = 10;
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
    em->i2c_ops_1.inc_use = em8300_i2c_inc;
    em->i2c_ops_1.dec_use = em8300_i2c_dec;
    em->i2c_ops_1.client_register = em8300_i2c_reg;
    em->i2c_ops_1.client_unregister = em8300_i2c_unreg;
    em->i2c_ops_1.data = em;
    
    ret = i2c_bit_add_bus(&em->i2c_ops_1);

    if(ret)
	return ret;

    /*
      Setup info structure for bus 2
    */

    em->i2c_data_2.setsda = &em8300_setsda;
    em->i2c_data_2.setscl = &em8300_setscl;
    em->i2c_data_2.getsda = &em8300_getsda;
    em->i2c_data_2.getscl = &em8300_getscl;
    em->i2c_data_2.mdelay = 10;
    em->i2c_data_2.udelay = 10;
    em->i2c_data_2.timeout = 100;

    pdata = kmalloc(sizeof(struct private_data_s),GFP_KERNEL);
    pdata->clk = 0x4;
    pdata->data = 0x8;
    pdata->em = em;
    
    em->i2c_data_2.data = pdata;

    strcpy(em->i2c_ops_2.name, "EM8300 I2C bus 2");
    em->i2c_ops_2.id = I2C_HW_B_EM8300;
    em->i2c_ops_2.algo = NULL;
    em->i2c_ops_2.algo_data = &em->i2c_data_2;
    em->i2c_ops_2.inc_use = em8300_i2c_inc;
    em->i2c_ops_2.dec_use = em8300_i2c_dec;
    em->i2c_ops_2.client_register = em8300_i2c_reg;
    em->i2c_ops_2.client_unregister = em8300_i2c_unreg;
    em->i2c_ops_2.data = em;
    
    ret = i2c_bit_add_bus(&em->i2c_ops_2);
    return ret;
}

void em8300_i2c_exit(struct em8300_s *em)
{
    /* unregister i2c_bus */
    kfree(em->i2c_data_1.data);
    kfree(em->i2c_data_2.data);
    i2c_bit_del_bus(&em->i2c_ops_1);
    i2c_bit_del_bus(&em->i2c_ops_2);
}

void em8300_clockgen_write(struct em8300_s *em, int abyte) {
    int i;

    em->mem[em->i2c_pin_reg] = 0x808;
    for(i=0; i < 8; i++) {
	em->mem[em->i2c_pin_reg] = 0x2000;
	em->mem[em->i2c_pin_reg] = 0x800 | ((abyte & 1) ? 8:0);
	em->mem[em->i2c_pin_reg] = 0x2020;
	abyte >>= 1;
    }

    em->mem[em->i2c_pin_reg] = 0x200;
    em->mem[em->i2c_pin_reg] = 0x202;
}	

