#include "stdafx.h"
#include "config.h"
#include "resource.h"
#include <algorithm>

static constexpr GUID guid_cfg_attach_art = { 0x18a6a6c2, 0xba3f, 0x4f13, { 0x8a, 0x08, 0xcd, 0xfa, 0xe3, 0x57, 0x20, 0x82 } };
static constexpr GUID guid_cfg_art_port = { 0x1d78b0eb, 0xcc83, 0x4bd9, { 0x9b, 0x88, 0x77, 0x65, 0x33, 0xf8, 0x76, 0xde } };
static constexpr GUID guid_cfg_art_max_edge = { 0x1ec461d5, 0x65b1, 0x4183, { 0x95, 0x1e, 0x37, 0x60, 0x8a, 0x7f, 0xa3, 0x99 } };
static constexpr GUID guid_preferences_page = { 0x29c963a2, 0x905c, 0x471b, { 0xbc, 0x81, 0xc8, 0xbb, 0x10, 0xb5, 0x3a, 0x68 } };

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
