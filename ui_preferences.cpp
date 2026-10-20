#include "stdafx.h"
#include "config.h"
#include "resource.h"
#include <algorithm>

static constexpr GUID guid_cfg_attach_art = { 0xfab8f1bf, 0xb3ad, 0x4c0e, { 0x8f, 0xb5, 0x6a, 0x7b, 0x3c, 0xf5, 0xeb, 0xd1 } };
static constexpr GUID guid_cfg_art_port = { 0xa3fbed8c, 0xbbb1, 0x4464, { 0x92, 0xa1, 0x1b, 0xb8, 0xdd, 0x2e, 0x09, 0x9b } };
static constexpr GUID guid_cfg_art_max_edge = { 0x5213f65c, 0x4c63, 0x4b45, { 0xa7, 0xff, 0xd0, 0x5d, 0x23, 0x79, 0x18, 0x39 } };
static constexpr GUID guid_preferences_page = { 0x8cd8c134, 0xe8be, 0x45c1, { 0xb9, 0x08, 0x5b, 0x53, 0xe9, 0x50, 0x26, 0xa5 } };

cfg_bool cfg_attach_art(guid_cfg_attach_art, default_attach_art);
cfg_uint cfg_art_port(guid_cfg_art_port, default_art_port);
cfg_uint cfg_art_max_edge(guid_cfg_art_max_edge, default_art_max_edge);

namespace
{
	// Plain Win32 dialog; the SDK's preferences_page_impl<> helper needs WTL.
	class preferences_instance : public preferences_page_instance
	{
	public:
		preferences_instance(HWND parent, preferences_page_callback::ptr callback) : m_callback(callback)
		{
			CreateDialogParam(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_PREFERENCES), parent, dialog_proc, reinterpret_cast<LPARAM>(this));
		}

		t_uint32 get_state() override
		{
			t_uint32 state = preferences_state::resettable;
			if (has_changed()) state |= preferences_state::changed;
			return state;
		}

		fb2k::hwnd_t get_wnd() override { return m_wnd; }

		void apply() override
		{
			cfg_attach_art = IsDlgButtonChecked(m_wnd, IDC_ATTACH_ART) == BST_CHECKED;
			cfg_art_port = read_number(IDC_ART_PORT, 1, 65535, (unsigned)cfg_art_port.get());
			cfg_art_max_edge = read_number(IDC_ART_MAX_EDGE, min_art_max_edge, max_art_max_edge, (unsigned)cfg_art_max_edge.get());
			// Show the values as clamped.
			show(cfg_attach_art, (unsigned)cfg_art_port.get(), (unsigned)cfg_art_max_edge.get());
			m_callback->on_state_changed();
		}

		void reset() override
		{
			show(default_attach_art, default_art_port, default_art_max_edge);
			m_callback->on_state_changed();
		}

	private:
		static INT_PTR CALLBACK dialog_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
		{
			if (msg == WM_INITDIALOG) {
				auto self = reinterpret_cast<preferences_instance*>(lp);
				SetWindowLongPtr(wnd, DWLP_USER, lp);
				self->m_wnd = wnd;
				self->show(cfg_attach_art, (unsigned)cfg_art_port.get(), (unsigned)cfg_art_max_edge.get());
				return FALSE;
			}
			auto self = reinterpret_cast<preferences_instance*>(GetWindowLongPtr(wnd, DWLP_USER));
			if (self && msg == WM_COMMAND && !self->m_showing) {
				switch (LOWORD(wp)) {
				case IDC_ATTACH_ART:
					if (HIWORD(wp) == BN_CLICKED) {
						self->update_enabled();
						self->m_callback->on_state_changed();
					}
					break;
				case IDC_ART_PORT:
				case IDC_ART_MAX_EDGE:
					if (HIWORD(wp) == EN_CHANGE) self->m_callback->on_state_changed();
					break;
				}
			}
			return FALSE;
		}

		void show(bool attach, unsigned port, unsigned max_edge)
		{
			m_showing = true;
			CheckDlgButton(m_wnd, IDC_ATTACH_ART, attach ? BST_CHECKED : BST_UNCHECKED);
			SetDlgItemInt(m_wnd, IDC_ART_PORT, port, FALSE);
			SetDlgItemInt(m_wnd, IDC_ART_MAX_EDGE, max_edge, FALSE);
			update_enabled();
			m_showing = false;
		}

		void update_enabled()
		{
			const BOOL attach = IsDlgButtonChecked(m_wnd, IDC_ATTACH_ART) == BST_CHECKED;
			EnableWindow(GetDlgItem(m_wnd, IDC_ART_PORT), attach);
			EnableWindow(GetDlgItem(m_wnd, IDC_ART_MAX_EDGE), attach);
		}

		unsigned read_number(int id, unsigned min, unsigned max, unsigned fallback)
		{
			BOOL ok = FALSE;
			UINT value = GetDlgItemInt(m_wnd, id, &ok, FALSE);
			return ok ? std::clamp<unsigned>(value, min, max) : fallback;
		}

		bool has_changed()
		{
			BOOL port_ok = FALSE, edge_ok = FALSE;
			UINT port = GetDlgItemInt(m_wnd, IDC_ART_PORT, &port_ok, FALSE);
			UINT edge = GetDlgItemInt(m_wnd, IDC_ART_MAX_EDGE, &edge_ok, FALSE);
			return (IsDlgButtonChecked(m_wnd, IDC_ATTACH_ART) == BST_CHECKED) != cfg_attach_art
				|| !port_ok || port != cfg_art_port.get()
				|| !edge_ok || edge != cfg_art_max_edge.get();
		}

		const preferences_page_callback::ptr m_callback;
		HWND m_wnd = nullptr;
		bool m_showing = false;
	};

	class preferences_page_myimpl : public preferences_page_v3
	{
	public:
		const char* get_name() override { return "foo_naupure"; }
		GUID get_guid() override { return guid_preferences_page; }
		GUID get_parent_guid() override { return guid_tools; }
		preferences_page_instance::ptr instantiate(fb2k::hwnd_t parent, preferences_page_callback::ptr callback) override
		{
			return fb2k::service_new<preferences_instance>(parent, callback);
		}
	};

	preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_factory;
}
