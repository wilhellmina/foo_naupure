#pragma once
#include "stdafx.h"

static const GUID guid_mygroup = { 0x3caf4d5c, 0xf516, 0x4e22, { 0x98, 0x0a, 0xa, 0xb0, 0x97, 0x47, 0x69, 0xe3 } };

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