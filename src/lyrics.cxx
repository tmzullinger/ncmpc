/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#include "lyrics.hxx"
#include "config.h"

#include <assert.h>

static PluginList empty, plugins;

void lyrics_init()
{
	plugin_list_load_directory(&plugins, LYRICS_PLUGIN_DIR);
}

PluginCycle *
lyrics_load(const char *artist, const char *title,
	    plugin_callback_t callback, void *data)
{
	const char *args[3] = { artist, title, nullptr };

	if (artist == nullptr || title == nullptr)
		return plugin_run(&empty, args, callback, data);

	return plugin_run(&plugins, args, callback, data);
}
