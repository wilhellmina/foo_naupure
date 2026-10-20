#pragma once
#include "stdafx.h"
#include <functional>

namespace artwork
{
	// If attaching album art is enabled, loads the front cover of `track` in a worker thread and
	// serves it to the userscript, then calls `open_intent` in the main thread.
	// `open_intent` is called whether or not the art could be prepared; with the option off it is called right away.
	void attach_then(metadb_handle_ptr track, std::function<void()> open_intent);
}
