/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * Workarounds for libncurses oddities.
 */

#ifndef NCFIX_H
#define NCFIX_H

#include "ncmpc_curses.h"

/**
 * Workaround for "comparison will always evaluate as 'true' for the
 * address of ...".  By wrapping the macro in this inline function,
 * gcc stops bitching about this.
 */
static inline int
fix_wattr_get(WINDOW *win, attr_t *attrs, short *pair, void *opts)
{
	(void)opts;

	return wattr_get(win, attrs, pair, opts);
}

#endif
