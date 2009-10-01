/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef SCREEN_H
#define SCREEN_H

#include "config.h"
#include "command.h"
#include "window.h"
#include "title_bar.h"
#include "progress_bar.h"
#include "status_bar.h"

#include <mpd/client.h>

#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

#define MAX_SONGNAME_LENGTH   512

struct mpdclient;
struct screen_functions;

struct screen {
	struct title_bar title_bar;
	struct window main_window;
	struct progress_bar progress_bar;
	struct status_bar status_bar;

	/* GTime is equivalent to time_t */
	GTime start_timestamp;

	unsigned cols, rows;

	char *buf;
	size_t buf_size;

	char *findbuf;
	GList *find_history;
};

extern struct screen screen;

void screen_init(struct mpdclient *c);
void screen_exit(void);
void screen_resize(struct mpdclient *c);
void screen_status_message(const char *msg);
void screen_status_printf(const char *format, ...);
void screen_paint(struct mpdclient *c);
void screen_update(struct mpdclient *c);
void screen_cmd(struct mpdclient *c, command_t cmd);

void
screen_switch(const struct screen_functions *sf, struct mpdclient *c);
void 
screen_swap(struct mpdclient *c, const struct mpd_song *song);

gboolean
screen_is_visible(const struct screen_functions *sf);

int
screen_get_mouse_event(struct mpdclient *c, unsigned long *bstate, int *row);

#endif
