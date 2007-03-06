/* $Id$
 *
 * em8300_eeprom.c -- read the eeprom on em8300-based boards
 * Copyright (C) 2007 Nicolas Boullis <nboullis@debian.org>
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

#include "em8300_eeprom.h"
#include <linux/em8300.h>
#include <linux/i2c.h>
#include <linux/crypto.h>
#include <linux/slab.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
static inline void *crypto_tfm_ctx(struct crypto_tfm *tfm)
{
	return (void *)&tfm[1];
}
#endif

int em8300_eeprom_read(struct em8300_s *em, u8 *data)
{
	struct i2c_msg message[] = {
		{
			.addr = 0x50,
			.flags = 0,
			.len = 1,
			.buf = "",
		},
		{
			.addr = 0x50,
			.flags = I2C_M_RD,
			.len = 256,
			.buf = data,
		}
	};

	if (i2c_transfer(&em->i2c_ops_2, message, 2) == 2)
		return 0;

	return -1;
}

int em8300_eeprom_checksum_init(struct em8300_s *em)
{
	u8 *buf;
	int err;
	struct crypto_tfm *tfm;

	buf = kmalloc(256, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	err = em8300_eeprom_read(em, buf);
	if (err != 0)
		goto cleanup;

	tfm = crypto_alloc_tfm("md5", 0);
	if (tfm == NULL) {
		err = -5;
		goto cleanup;
	}

	em->eeprom_checksum = kmalloc(16, GFP_KERNEL);
	if (em->eeprom_checksum == NULL) {
		err = -ENOMEM;
		goto cleanup;
	}

	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->__crt_alg->cra_digest.dia_init(crypto_tfm_ctx(tfm));
	tfm->__crt_alg->cra_digest.dia_update(crypto_tfm_ctx(tfm),
					      buf, 128);
	tfm->__crt_alg->cra_digest.dia_final(crypto_tfm_ctx(tfm),
					     em->eeprom_checksum);
	crypto_free_tfm(tfm);

 cleanup:
	kfree(buf);

	return err;
}

void em8300_eeprom_checksum_deinit(struct em8300_s *em)
{
	if (em->eeprom_checksum) {
		kfree(em->eeprom_checksum);
		em->eeprom_checksum = NULL;
	}
}
