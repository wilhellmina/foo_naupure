#pragma once
#include "stdafx.h"

// Settings edited on the preferences page (preferences.cpp).
extern cfg_string cfg_post_format;
extern cfg_bool cfg_attach_art;
extern cfg_uint cfg_art_port;
extern cfg_uint cfg_art_max_edge;

// foobar2000 title formatting, one line per line of the post. Stored with LF line breaks.
// Parentheses are special in title formatting, hence '(' and ')'.
constexpr const char* default_post_format = "#NowPlaying\n%title%['('%artist%')'][ / %album%]";

constexpr bool default_attach_art = true;
// Must match PORT in the userscript.
constexpr unsigned default_art_port = 18080;
constexpr unsigned default_art_max_edge = 1600;

constexpr unsigned min_art_max_edge = 200;
constexpr unsigned max_art_max_edge = 4096;
