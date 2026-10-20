#pragma once
#include "stdafx.h"

// Settings edited on the preferences page (ui_preferences.cpp).
extern cfg_bool cfg_attach_art;
extern cfg_uint cfg_art_port;
extern cfg_uint cfg_art_max_edge;

constexpr bool default_attach_art = true;
// Must match PORT in the userscript.
constexpr unsigned default_art_port = 18080;
constexpr unsigned default_art_max_edge = 1600;

constexpr unsigned min_art_max_edge = 200;
constexpr unsigned max_art_max_edge = 4096;
