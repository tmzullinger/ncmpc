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

#ifndef NCMPC_STATUS_BAR_HXX
#define NCMPC_STATUS_BAR_HXX

#include "Window.hxx"

#ifndef NCMPC_MINI
#include "hscroll.hxx"
#endif

#include <mpd/status.h>

#include <string>

struct mpd_status;
struct mpd_song;

class StatusBar {
	Window window;

	unsigned message_source_id = 0;

#ifndef NCMPC_MINI
	class hscroll hscroll;
#endif

	const char *left_text;
	char right_text[64];

	std::string center_text;

	unsigned left_width, right_width;
#ifndef NCMPC_MINI
	unsigned center_width;
#endif

public:
	StatusBar(Point p, unsigned width);
	~StatusBar();

	Window &GetWindow() {
		return window;
	}

	void SetMessage(const char *msg);
	void ClearMessage();

	void OnResize(Point p, unsigned width);
	void Update(const struct mpd_status *status,
		    const struct mpd_song *song);
	void Paint() const;

private:
	bool OnClearMessageTimer();
};

#endif
