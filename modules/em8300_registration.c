/* $Id$
 *
 * em8300_registration.c -- common interface for everything that needs
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

#include "em8300_registration.h"


static struct em8300_registrar_s *registrars[] =
{
	NULL
};


void em8300_register_driver(void)
{
	int i;
	for (i = 0; registrars[i]; i++) {
		if (registrars[i]->register_driver)
			registrars[i]->register_driver();
	}
}

void em8300_register_card(struct em8300_s *em)
{
	int i;
	for (i = 0; registrars[i]; i++) {
		if (registrars[i]->register_card)
			registrars[i]->register_card(em);
	}
}

void em8300_enable_card(struct em8300_s *em)
{
	int i;
	for (i = 0; registrars[i]; i++) {
		if (registrars[i]->enable_card)
			registrars[i]->enable_card(em);
	}
}

void em8300_disable_card(struct em8300_s *em)
{
	int i;
	for (i = 0; registrars[i]; i++) {
		if (registrars[i]->disable_card)
			registrars[i]->disable_card(em);
	}
}

void em8300_unregister_card(struct em8300_s *em)
{
	int i;
	for (i = 0; registrars[i]; i++) {
		if (registrars[i]->unregister_card)
			registrars[i]->unregister_card(em);
	}
}

void em8300_unregister_driver(void)
{
	int i;
	for (i = 0; registrars[i]; i++) {
		if (registrars[i]->unregister_driver)
			registrars[i]->unregister_driver();
	}
}
