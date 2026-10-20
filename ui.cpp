#include "stdafx.h"

static const GUID guid_mygroup = { 0x572de7f4, 0xcbdf, 0x479a, { 0x97, 0x26, 0xa, 0xb0, 0x97, 0x47, 0x69, 0xe3 } };

class MainMenuCommandsImpl : public mainmenu_commands
{
public:
	t_uint32 get_command_count() override;
	GUID get_command(t_uint32 p_index) override;
	void get_name(t_uint32 p_index, pfc::string_base& p_out) override;
	bool get_description(t_uint32 /* p_index */, pfc::string_base& p_out) override;
	GUID get_parent() override;
	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override;
	bool get_display(t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags) override;
};

t_uint32 MainMenuCommandsImpl::get_command_count()
{
	return 1;
}

GUID MainMenuCommandsImpl::get_command(t_uint32 p_index)
{
	switch (p_index)
	{
	case 0:
		return guid_mygroup;
	default:
		uBugCheck();
	}
}

void MainMenuCommandsImpl::get_name(t_uint32 p_index, pfc::string_base& p_out)
{
	switch (p_index)
	{
	case 0:
		p_out = "tweet";
		return;
	default:
		uBugCheck();
	}
}

bool MainMenuCommandsImpl::get_description(t_uint32 /* p_index */, pfc::string_base& p_out)
{
	p_out = "tweet via browser";
	return true;
}

GUID MainMenuCommandsImpl::get_parent()
{
	return mainmenu_groups::playback;
}

void MainMenuCommandsImpl::execute(t_uint32 p_index, service_ptr_t<service_base> p_callback)
{
	
}

bool MainMenuCommandsImpl::get_display(t_uint32 p_index, pfc::string_base& p_out, t_uint32& p_flags)
{
	get_name(p_index, p_out);
	p_flags = sort_priority_dontcare;
	return true;
}

mainmenu_commands_factory_t<MainMenuCommandsImpl> g_mainmenuCommands;