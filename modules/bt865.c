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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/malloc.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/signal.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <asm/segment.h>
#include <linux/types.h>
#include <linux/wrapper.h>

#include <linux/videodev.h>
#include <linux/version.h>
#include <asm/uaccess.h>

#include <linux/i2c-algo-bit.h>
#include <linux/video_encoder.h>

#include "bt865.h"

#include "encoder.h"

#define DEBUG(x) x

#define i2c_is_isa_client(clientptr) \
        ((clientptr)->adapter->algo->id == I2C_ALGO_ISA)
#define i2c_is_isa_adapter(adapptr) \
        ((adapptr)->algo->id == I2C_ALGO_ISA)


static int bt865_attach_adapter(struct i2c_adapter *adapter);
int bt865_detach_client(struct i2c_client *client);
int bt865_command(struct i2c_client *client, unsigned int cmd, void *arg);
void bt865_inc_use (struct i2c_client *client);
void bt865_dec_use (struct i2c_client *client);

struct bt865_data_s {
    int chiptype;
    int mode;
    int bars;
    int enableoutput;

    unsigned char config[32];
    int configlen;
};

/* This is the driver that will be inserted */
static struct i2c_driver bt865_driver = {
  /* name */            "BT865 video encoder driver",
  /* id */              I2C_DRIVERID_BT865,
  /* flags */           I2C_DF_NOTIFY,
  /* attach_adapter */  &bt865_attach_adapter,
  /* detach_client */   &bt865_detach_client,
  /* command */         &bt865_command,
  /* inc_use */         &bt865_inc_use,
  /* dec_use */         &bt865_dec_use
};

int bt865_id = 0;

static
int bt865_setmode(int mode, struct i2c_client *client) {
    struct bt865_data_s *data = client->data ;
    unsigned char *config=NULL;

    DEBUG(printk("bt865_setmode(%d,%p)\n",mode,client));
    
    switch(mode) {
    case ENCODER_MODE_PAL:
	printk("<1>bt865.o: Configuring for PAL\n");
	i2c_smbus_write_byte_data(client,0xcc, 0xe4);
	i2c_smbus_write_byte_data(client,0xd0, 0x0);
	break;
    case ENCODER_MODE_PAL_M:
	printk("<1>bt865.o: Configuring for PALM\n");
	break;
    case ENCODER_MODE_PAL60:
	printk("<1>bt865.o: Configuring for PAL 60\n");
	break;
    case ENCODER_MODE_NTSC:
	printk("<1>bt865.o: Configuring for NTSC\n");
	i2c_smbus_write_byte_data(client,0xcc, 0x80);
	i2c_smbus_write_byte_data(client,0xd0, 0x0);
	break;
    default:
	return -1;
    }

    data->mode = mode;

    if(config) 
	memcpy(data->config, config, data->configlen);

    return 0;
}

static
int bt865_setup(struct i2c_client *client) {
    struct bt865_data_s *data = client->data ;
    
    memset(data->config, 0, sizeof(data->config));

    data->bars=0;
    data->enableoutput=0;

    /* Register settings comes from the DXR2 BT865 driver */
    
    // reset the chip
    i2c_smbus_write_byte_data(client,0xa6, 0x80);

  // set TXHS[7:0] to 0
    
    i2c_smbus_write_byte_data(client,0xac, 0x0);

  // set TXHE[7:0] to 0

    i2c_smbus_write_byte_data(client,0xae, 0x0);

  // set TXHS[10:8], TXHE[10:8], LUMADLY[1:0] to 0

    i2c_smbus_write_byte_data(client,0xb0, 0x0);

  // basically, disable teletext

    i2c_smbus_write_byte_data(client,0xb2, 0x10);

    // sets a reserved bit in register 0xBC!!!

    i2c_smbus_write_byte_data(client,0xbc, 0x10);


  // noninterlaced mode, setup off, among other things
    
    i2c_smbus_write_byte_data(client,0xcc, 0x42);

  // normal video mode, ESTATUS = 0
    
    i2c_smbus_write_byte_data(client,0xce, 0x2);

  // enable WSS/CGMS in second field

    i2c_smbus_write_byte_data(client,0xa0, 0x80);
    
    return 0;
}
static int bt865_detect(struct i2c_adapter *adapter, int address)
{
    struct bt865_data_s *data;
    struct i2c_client *new_client;
    int err;

    if(i2c_is_isa_adapter(adapter)) {
	printk("bt865a.o: called for an ISA bus adapter?!?\n");
	return 0;
    }

     if (! i2c_check_functionality(adapter,I2C_FUNC_SMBUS_READ_BYTE| 
                                        I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
	 return 0;

     
     
     if (! (new_client = kmalloc(sizeof(struct i2c_client) +
				 sizeof(struct bt865_data_s),
				 GFP_KERNEL))) 
	 return -ENOMEM;

     data = (struct bt865_data_s *) (((struct i2c_client *) new_client) + 1);
     new_client->addr = address;
     new_client->data = data;
     new_client->adapter = adapter;
     new_client->driver = &bt865_driver;
     new_client->flags = 0;

     i2c_smbus_write_byte_data(new_client,0xa6, 0xb1);

     if(i2c_smbus_read_byte_data(new_client,0) == 0xb1) {
	 printk("bt865.o: BT865 chip detected\n");

	 new_client->id = bt865_id++;

	 if ((err = i2c_attach_client(new_client))) {
	     kfree(new_client);
	     return err;
	 }

	 bt865_setup(new_client);

	 return 0;
     }
     
     kfree(new_client);
     return 0;              
}

static int bt865_attach_adapter(struct i2c_adapter *adapter)
{
    bt865_detect(adapter,0x45);
    return 0;
}


int bt865_detach_client(struct i2c_client *client)
{
    int err;

    if ((err = i2c_detach_client(client))) {
	printk("bt865.o: Client deregistration failed, client not detached.\n");
	return err;
    }

    kfree(client);

    return 0;
}

int bt865_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
    struct bt865_data_s *data = client->data ;

    switch(cmd) {
    case ENCODER_CMD_SETMODE:
	bt865_setmode((int)arg,client);
	break;
    case ENCODER_CMD_ENABLEOUTPUT:
	data->enableoutput = (int)arg;
	break;
    default:
    }
    return 0;
}

/* Nothing here yet */
void bt865_inc_use (struct i2c_client *client)
{
#ifdef MODULE
  MOD_INC_USE_COUNT;
#endif
}

/* Nothing here yet */
void bt865_dec_use (struct i2c_client *client)
{
#ifdef MODULE
  MOD_DEC_USE_COUNT;
#endif
}

/* ----------------------------------------------------------------------- */


#ifdef MODULE
int init_module(void)
#else
int bt865_init(void)
#endif
{
    return i2c_add_driver(&bt865_driver);
}



#ifdef MODULE

void cleanup_module(void)
{
    i2c_del_driver(&bt865_driver);
}

#endif
