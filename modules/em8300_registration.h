/* $Id$
 *
 * em8300_registration.h -- common interface for everything that needs
 *                          to be registered
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

#ifndef EM8300_REGISTRATION_H
#define EM8300_REGISTRATION_H

#include <linux/config.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/time.h>
#include <linux/em8300.h>

struct em8300_registrar_s {
	void (*register_driver)(void);
	void (*register_card)(struct em8300_s *);
	void (*enable_card)(struct em8300_s *);
	void (*disable_card)(struct em8300_s *);
	void (*unregister_card)(struct em8300_s *);
	void (*unregister_driver)(void);
	void (*audio_interrupt)(struct em8300_s *);
	void (*video_interrupt)(struct em8300_s *);
	void (*vbl_interrupt)(struct em8300_s *);
};

extern void em8300_register_driver(void);
extern void em8300_register_card(struct em8300_s *);
extern void em8300_enable_card(struct em8300_s *);
extern void em8300_disable_card(struct em8300_s *);
extern void em8300_unregister_card(struct em8300_s *);
extern void em8300_unregister_driver(void);
extern void em8300_audio_interrupt(struct em8300_s *);
extern void em8300_video_interrupt(struct em8300_s *);
extern void em8300_vbl_interrupt(struct em8300_s *);

#endif /* EM8300_REGISTRATION_H */
