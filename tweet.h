#pragma once
#include "stdafx.h"
#include <string>

namespace Kokura_Asahi
{
	//encode UTF8 to URI with percents
	//RFC3986
	std::string percentEnc(std::string str);

	//format the now playing track with the post text format (foobar2000 title formatting, one line per line of the post)
	//returns false when nothing is playing
	bool format_post(const char* format, std::string& out);

	void tweet(const std::string& text);
}
