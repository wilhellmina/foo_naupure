#pragma once
#include "stdafx.h"
#include <string>

namespace Kokura_Asahi
{
	//encode UTF8 to URI with percents 
	//RFC3986
	std::string percentEnc(std::string str);

	void tweet(file_info_impl *track_info);
}
