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

#include "SongPage.hxx"
#include "screen_interface.hxx"
#include "ListPage.hxx"
#include "ListText.hxx"
#include "TextListRenderer.hxx"
#include "FileBrowserPage.hxx"
#include "LyricsPage.hxx"
#include "screen_find.hxx"
#include "i18n.h"
#include "screen.hxx"
#include "charset.hxx"
#include "time_format.hxx"
#include "mpdclient.hxx"
#include "util/Macros.hxx"
#include "util/StringStrip.hxx"

#include <mpd/client.h>

#include <glib/gprintf.h>

#include <vector>
#include <string>

#include <assert.h>
#include <string.h>
#include <time.h>

enum {
	LABEL_LENGTH = MPD_TAG_COUNT,
	LABEL_PATH,
	LABEL_BITRATE,
	LABEL_FORMAT,
	LABEL_POSITION,
};

struct tag_label {
	unsigned tag_type;
	const char *label;
};

static const struct tag_label tag_labels[] = {
	{ MPD_TAG_ARTIST, N_("Artist") },
	{ MPD_TAG_TITLE, N_("Title") },
	{ MPD_TAG_ALBUM, N_("Album") },
	{ LABEL_LENGTH, N_("Length") },
	{ LABEL_POSITION, N_("Position") },
	{ MPD_TAG_COMPOSER, N_("Composer") },
	{ MPD_TAG_NAME, N_("Name") },
	{ MPD_TAG_DISC, N_("Disc") },
	{ MPD_TAG_TRACK, N_("Track") },
	{ MPD_TAG_DATE, N_("Date") },
	{ MPD_TAG_GENRE, N_("Genre") },
	{ MPD_TAG_COMMENT, N_("Comment") },
	{ LABEL_PATH, N_("Path") },
	{ LABEL_BITRATE, N_("Bitrate") },
	{ LABEL_FORMAT, N_("Format") },
	{ 0, nullptr }
};

static unsigned max_tag_label_width;

enum stats_label {
	STATS_ARTISTS,
	STATS_ALBUMS,
	STATS_SONGS,
	STATS_UPTIME,
	STATS_DBUPTIME,
	STATS_PLAYTIME,
	STATS_DBPLAYTIME,
};

static const char *const stats_labels[] = {
	N_("Number of artists"),
	N_("Number of albums"),
	N_("Number of songs"),
	N_("Uptime"),
	N_("Most recent db update"),
	N_("Playtime"),
	N_("DB playtime"),
};

static unsigned max_stats_label_width;

static struct mpd_song *next_song;

class SongPage final : public ListPage, ListText {
	ScreenManager &screen;

	mpd_song *selected_song = nullptr;
	mpd_song *played_song = nullptr;

	std::vector<std::string> lines;

public:
	SongPage(ScreenManager &_screen, WINDOW *w, Size size)
		:ListPage(w, size),
		 screen(_screen) {
		lw.hide_cursor = true;
	}

	~SongPage() override {
		Clear();
	}

private:
	void Clear();

	/**
	 * Appends a line with a fixed width for the label column.
	 * Handles nullptr strings gracefully.
	 */
	void AppendLine(const char *label, const char *value,
			unsigned label_col);

	void AppendTag(const struct mpd_song *song, enum mpd_tag_type tag);
	void AddSong(const struct mpd_song *song);
	void AppendStatsLine(enum stats_label label, const char *value);
	bool AddStats(struct mpd_connection *connection);

public:
	/* virtual methods from class Page */
	void OnClose() override {
		Clear();
	}

	void Paint() const override;
	void Update(struct mpdclient &c, unsigned events) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;

private:
	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const override;
};

void
SongPage::Clear()
{
	lines.clear();

	if (selected_song != nullptr) {
		mpd_song_free(selected_song);
		selected_song = nullptr;
	}
	if (played_song != nullptr) {
		mpd_song_free(played_song);
		played_song = nullptr;
	}
}

const char *
SongPage::GetListItemText(char *, size_t, unsigned idx) const
{
	return lines[idx].c_str();
}

static Page *
screen_song_init(ScreenManager &_screen, WINDOW *w, Size size)
{
	for (unsigned i = 0; tag_labels[i].label != nullptr; ++i) {
		unsigned width = utf8_width(_(tag_labels[i].label));
		if (width > max_tag_label_width)
			max_tag_label_width = width;
	}

	for (unsigned i = 0; i < ARRAY_SIZE(stats_labels); ++i) {
		if (stats_labels[i] != nullptr) {
			unsigned width = utf8_width(_(stats_labels[i]));

			if (width > max_stats_label_width)
				max_stats_label_width = width;
		}
	}

	return new SongPage(_screen, w, size);
}

const char *
SongPage::GetTitle(gcc_unused char *str, gcc_unused size_t size) const
{
	return _("Song viewer");
}

void
SongPage::Paint() const
{
	lw.Paint(TextListRenderer(*this));
}

void
SongPage::AppendLine(const char *label, const char *value, unsigned label_col)
{
	const unsigned label_width = locale_width(label) + 2;

	assert(label != nullptr);
	assert(value != nullptr);
	assert(g_utf8_validate(value, -1, nullptr));

	/* +2 for ': ' */
	label_col += 2;
	const int value_col = lw.size.width - label_col;
	/* calculate the number of required linebreaks */
	const gchar *value_iter = value;
	const size_t label_length = strlen(label);
	const size_t label_size = label_length + label_col;

	while (*value_iter != 0) {
		char *entry = (char *)g_malloc(label_size), *entry_iter;
		if (value_iter == value) {
			memcpy(entry, label, label_length);
			entry_iter = entry + label_length;
			*entry_iter++ = ':';
			/* fill the label column with whitespaces */
			size_t n_space = label_col - label_width + 1;
			memset(entry_iter, ' ', n_space);
			entry_iter += n_space;
		}
		else {
			/* fill the label column with whitespaces */
			memset(entry, ' ', label_col);
			entry_iter = entry + label_col;
		}
		/* skip whitespaces */
		value_iter = StripLeft(value_iter);

		char *p = g_strdup(value_iter);
		unsigned width = utf8_cut_width(p, value_col);
		if (width == 0) {
			/* not enough room for anything - bail out */
			g_free(entry);
			g_free(p);
			break;
		}

		*entry_iter = 0;

		value_iter += strlen(p);
		p = replace_utf8_to_locale(p);
		char *q = g_strconcat(entry, p, nullptr);
		g_free(entry);
		g_free(p);

		lines.emplace_back(q);
		g_free(q);
	}
}

gcc_pure
static const char *
get_tag_label(unsigned tag)
{
	for (unsigned i = 0; tag_labels[i].label != nullptr; ++i)
		if (tag_labels[i].tag_type == tag)
			return _(tag_labels[i].label);

	assert(tag < MPD_TAG_COUNT);
	return mpd_tag_name((enum mpd_tag_type)tag);
}

void
SongPage::AppendTag(const struct mpd_song *song, enum mpd_tag_type tag)
{
	const char *label = get_tag_label(tag);
	unsigned i = 0;
	const char *value;

	assert((unsigned)tag < ARRAY_SIZE(tag_labels));
	assert(label != nullptr);

	while ((value = mpd_song_get_tag(song, tag, i++)) != nullptr)
		AppendLine(label, value, max_tag_label_width);
}

void
SongPage::AddSong(const struct mpd_song *song)
{
	assert(song != nullptr);

	char songpos[16];
	snprintf(songpos, sizeof(songpos), "%d", mpd_song_get_pos(song) + 1);
	AppendLine(get_tag_label(LABEL_POSITION), songpos,
		   max_tag_label_width);

	AppendTag(song, MPD_TAG_ARTIST);
	AppendTag(song, MPD_TAG_TITLE);
	AppendTag(song, MPD_TAG_ALBUM);

	/* create time string and add it */
	if (mpd_song_get_duration(song) > 0) {
		char length[16];
		format_duration_short(length, sizeof(length),
				      mpd_song_get_duration(song));

		const char *value = length;

		char buffer[64];

		if (mpd_song_get_end(song) > 0) {
			char start[16], end[16];
			format_duration_short(start, sizeof(start),
					      mpd_song_get_start(song));
			format_duration_short(end, sizeof(end),
					      mpd_song_get_end(song));

			snprintf(buffer, sizeof(buffer), "%s [%s-%s]\n",
				 length, start, end);
			value = buffer;
		} else if (mpd_song_get_start(song) > 0) {
			char start[16];
			format_duration_short(start, sizeof(start),
					      mpd_song_get_start(song));

			snprintf(buffer, sizeof(buffer), "%s [%s-]\n",
				 length, start);
			value = buffer;
		}

		AppendLine(get_tag_label(LABEL_LENGTH), value,
				   max_tag_label_width);
	}

	AppendTag(song, MPD_TAG_COMPOSER);
	AppendTag(song, MPD_TAG_NAME);
	AppendTag(song, MPD_TAG_DISC);
	AppendTag(song, MPD_TAG_TRACK);
	AppendTag(song, MPD_TAG_DATE);
	AppendTag(song, MPD_TAG_GENRE);
	AppendTag(song, MPD_TAG_COMMENT);

	AppendLine(get_tag_label(LABEL_PATH), mpd_song_get_uri(song),
		   max_tag_label_width);
}

void
SongPage::AppendStatsLine(enum stats_label label, const char *value)
{
	AppendLine(_(stats_labels[label]), value,
			   max_stats_label_width);
}

bool
SongPage::AddStats(struct mpd_connection *connection)
{
	struct mpd_stats *mpd_stats = mpd_run_stats(connection);
	if (mpd_stats == nullptr)
		return false;

	lines.emplace_back(_("MPD statistics"));

	char buf[64];
	snprintf(buf, sizeof(buf), "%d",
		 mpd_stats_get_number_of_artists(mpd_stats));
	AppendStatsLine(STATS_ARTISTS, buf);
	snprintf(buf, sizeof(buf), "%d",
		 mpd_stats_get_number_of_albums(mpd_stats));
	AppendStatsLine(STATS_ALBUMS, buf);
	snprintf(buf, sizeof(buf), "%d",
		 mpd_stats_get_number_of_songs(mpd_stats));
	AppendStatsLine(STATS_SONGS, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_db_play_time(mpd_stats));
	AppendStatsLine(STATS_DBPLAYTIME, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_play_time(mpd_stats));
	AppendStatsLine(STATS_PLAYTIME, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_uptime(mpd_stats));
	AppendStatsLine(STATS_UPTIME, buf);

	const time_t t = mpd_stats_get_db_update_time(mpd_stats);
	strftime(buf, sizeof(buf), "%x", localtime(&t));
	AppendStatsLine(STATS_DBUPTIME, buf);

	mpd_stats_free(mpd_stats);
	return true;
}

static void
audio_format_to_string(char *buffer, size_t size,
		       const struct mpd_audio_format *format)
{
#if LIBMPDCLIENT_CHECK_VERSION(2,10,0)
	if (format->bits == MPD_SAMPLE_FORMAT_FLOAT) {
		snprintf(buffer, size, "%u:f:%u",
			 format->sample_rate,
			 format->channels);
		return;
	}

	if (format->bits == MPD_SAMPLE_FORMAT_DSD) {
		if (format->sample_rate > 0 &&
		    format->sample_rate % 44100 == 0) {
			/* use shortcuts such as "dsd64" which implies the
			   sample rate */
			snprintf(buffer, size, "dsd%u:%u",
				 format->sample_rate * 8 / 44100,
				 format->channels);
			return;
		}

		snprintf(buffer, size, "%u:dsd:%u",
			 format->sample_rate,
			 format->channels);
		return;
	}
#endif

	snprintf(buffer, size, "%u:%u:%u",
		 format->sample_rate, format->bits,
		 format->channels);
}

void
SongPage::Update(struct mpdclient &c, unsigned)
{
	lines.clear();

	/* If a song was selected before the song screen was opened */
	if (next_song != nullptr) {
		assert(selected_song == nullptr);
		selected_song = next_song;
		next_song = nullptr;
	}

	if (selected_song != nullptr &&
	    (c.song == nullptr ||
	     strcmp(mpd_song_get_uri(selected_song),
		    mpd_song_get_uri(c.song)) != 0 ||
	     !c.playing_or_paused)) {
		lines.emplace_back(_("Selected song"));
		AddSong(selected_song);
		lines.emplace_back(std::string());
	}

	if (c.song != nullptr && c.playing_or_paused) {
		if (played_song != nullptr)
			mpd_song_free(played_song);

		played_song = mpd_song_dup(c.song);
		lines.emplace_back(_("Currently playing song"));
		AddSong(played_song);

		if (mpd_status_get_kbit_rate(c.status) > 0) {
			char buf[16];
			snprintf(buf, sizeof(buf), _("%d kbps"),
				 mpd_status_get_kbit_rate(c.status));
			AppendLine(get_tag_label(LABEL_BITRATE), buf,
				   max_tag_label_width);
		}

		const struct mpd_audio_format *format =
			mpd_status_get_audio_format(c.status);
		if (format) {
			char buf[32];
			audio_format_to_string(buf, sizeof(buf), format);
			AppendLine(get_tag_label(LABEL_FORMAT), buf,
				   max_tag_label_width);
		}

		lines.emplace_back(std::string());
	}

	/* Add some statistics about mpd */
	auto *connection = c.GetConnection();
	if (connection != nullptr && !AddStats(connection))
		c.HandleError();

	lw.SetLength(lines.size());
	SetDirty();
}

bool
SongPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case CMD_LOCATE:
		if (selected_song != nullptr) {
			screen_file_goto_song(screen, c, *selected_song);
			return true;
		}
		if (played_song != nullptr) {
			screen_file_goto_song(screen, c, *played_song);
			return true;
		}

		return false;

#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		if (selected_song != nullptr) {
			screen_lyrics_switch(screen, c, *selected_song, false);
			return true;
		}
		if (played_song != nullptr) {
			screen_lyrics_switch(screen, c, *played_song, true);
			return true;
		}
		return false;

#endif

	case CMD_SCREEN_SWAP:
		if (selected_song != nullptr)
			screen.Swap(c, selected_song);
		else
		// No need to check if this is null - we'd pass null anyway
			screen.Swap(c, played_song);
		return true;

	default:
		break;
	}

	if (screen_find(screen, &lw, cmd, *this)) {
		/* center the row */
		lw.Center(lw.selected);
		SetDirty();
		return true;
	}

	return false;
}

const struct screen_functions screen_song = {
	"song",
	screen_song_init,
};

void
screen_song_switch(ScreenManager &_screen, struct mpdclient &c,
		   const struct mpd_song &song)
{
	next_song = mpd_song_dup(&song);
	_screen.Switch(screen_song, c);
}
