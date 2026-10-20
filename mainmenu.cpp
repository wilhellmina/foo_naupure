#include "stdafx.h"
#include "mainmenu.h"
#include "tweet.h"
#include "artwork.h"
#include "config.h"

//impl tweet button
t_uint32 Tweet_Button_impl::get_command_count()
{
	return 1;
}

GUID Tweet_Button_impl::get_command(t_uint32 p_index)
{
	switch (p_index)
	{
	case 0:
		return guid_mygroup;
	default:
		uBugCheck();
	}
}

void Tweet_Button_impl::get_name(t_uint32 p_index, pfc::string_base& p_out)
{
	switch (p_index)
	{
	case 0:
		p_out = "Tweet";
		return;
	default:
		uBugCheck();
	}
}

bool Tweet_Button_impl::get_description(t_uint32 /* p_index */, pfc::string_base& p_out)
{
	p_out = "Tweet Nowplaying";
	return true;
}

GUID Tweet_Button_impl::get_parent()
{
	return mainmenu_groups::playback;
}

void Tweet_Button_impl::execute(t_uint32 p_index, service_ptr_t<service_base> p_callback)
{
	//take track info only when button is clicked, not on new track, because it may be wrong info if user click button before new track is loaded
	metadb_handle_ptr track_handle;
	std::string text;

	if (playback_control::get()->get_now_playing(track_handle) && Kokura_Asahi::format_post(cfg_post_format.get().c_str(), text)) {
		//the intent page is opened once the album art (if enabled) is ready for the userscript to pick up
		artwork::attach_then(track_handle, [text]() { Kokura_Asahi::tweet(text); });
	}
	else
	{
		return;
	}
}

bool Tweet_Button_impl::get_display(t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags)
{
	get_name(p_index, p_out);
	p_flags = 0;
	return true;
}

mainmenu_commands_factory_t<Tweet_Button_impl> g_mainmenuCommands;