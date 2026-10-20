#include "stdafx.h"
#include "tweet.h"
#include <sstream>
#include <Windows.h>
#include <iomanip>
#include <algorithm>

#include <stdio.h>
#include <sys/stat.h>


namespace Kokura_Asahi
{
	//encode UTF8 to URI with percents 
	//RFC3986
	std::string percentEnc(std::string str) {
		const int NUM_BEGIN_UTF8 = 48;
		const int CAPITAL_BEGIN_UTF8 = 65;
		const int LOWER_BEGIN_UTF8 = 97;

		int charCode = -1;
		std::string encoded;
		std::stringstream out;

		//for文で1byteずつストリームに入れていく
		for (int i = 0; str[i] != 0; i++) {
			//文字列中の1byte分のデータを整数値として代入
			charCode = (int)(unsigned char)str[i];

			//エンコードする必要の無い文字の判定
			if ((NUM_BEGIN_UTF8 <= charCode && charCode <= NUM_BEGIN_UTF8 + 9)
				|| (CAPITAL_BEGIN_UTF8 <= charCode && charCode <= CAPITAL_BEGIN_UTF8 + 25)
				|| (LOWER_BEGIN_UTF8 <= charCode && charCode <= LOWER_BEGIN_UTF8 + 25)
				|| str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '~')
			{
				//エンコードの必要が無い文字はそのままストリームに入れる
				out << str[i];
			}
			else {
				//エンコードする場合は%記号と文字コードの16進数表示をストリームに入れる
				out << '%' << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << charCode;
			}
		}
		//ストリームの文字列をstringのインスタンスに代入しreturn
		encoded = out.str();
		return encoded;
	}	


	bool format_post(const char* format, std::string& out) {
		auto playback = playback_control::get();
		auto compiler = titleformat_compiler::get();
		std::string text;

		//each line of the format is compiled on its own, so the format can be written the way the post should look
		std::istringstream lines(format);
		std::string line;
		for (bool first = true; std::getline(lines, line); first = false) {
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			titleformat_object::ptr script;
			compiler->compile_safe_ex(script, line.c_str());
			pfc::string8 formatted;
			if (!playback->playback_format_title(nullptr, formatted, script, nullptr, playback_control::display_level_all)) {
				return false;
			}
			if (!first) {
				text += '\n';
			}
			text += formatted.c_str();
		}

		//$crlf() gives CRLF; the post uses LF only
		text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
		out = std::move(text);
		return true;
	}


	void tweet(const std::string& text) {
		std::string data_encoded = "https://twitter.com/intent/tweet?text=" + percentEnc(text);



		//shellexecute
		HINSTANCE ret = ShellExecuteA(0, "open", data_encoded.c_str(), NULL, NULL, SW_SHOWNORMAL);
		//error handling
	}
}