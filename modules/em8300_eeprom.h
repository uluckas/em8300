/*
 * em8300_eeprom.h -- read the eeprom on em8300-based boards
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
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _EM8300_EEPROM_H
#define _EM8300_EEPROM_H

#include <linux/types.h>

struct em8300_s;

extern int em8300_eeprom_read(struct em8300_s *em, u8 *data);

extern int em8300_eeprom_checksum_init(struct em8300_s *em);

extern void em8300_eeprom_checksum_deinit(struct em8300_s *em);

#endif /* _EM8300_EEPROM_H */
