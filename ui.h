#pragma once
#include "stdafx.h"

static const GUID guid_mygroup = { 0x470ee1ed, 0xbcdf, 0x4087, { 0x92, 0xa9, 0x51, 0x17, 0x91, 0x6a, 0xe2, 0xc4 } };

class Tweet_Button_impl : public mainmenu_commands
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