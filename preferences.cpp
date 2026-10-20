#include "stdafx.h"
#include "config.h"
#include "resource.h"
#include "tweet.h"
#include <algorithm>
#include <string>

static constexpr GUID guid_cfg_post_format = { 0x81a3c992, 0xd134, 0x4f73, { 0x8b, 0x60, 0xa4, 0xb9, 0xb5, 0x81, 0x25, 0x4e } };
static constexpr GUID guid_cfg_attach_art = { 0x18a6a6c2, 0xba3f, 0x4f13, { 0x8a, 0x08, 0xcd, 0xfa, 0xe3, 0x57, 0x20, 0x82 } };
static constexpr GUID guid_cfg_art_port = { 0x1d78b0eb, 0xcc83, 0x4bd9, { 0x9b, 0x88, 0x77, 0x65, 0x33, 0xf8, 0x76, 0xde } };
static constexpr GUID guid_cfg_art_max_edge = { 0x1ec461d5, 0x65b1, 0x4183, { 0x95, 0x1e, 0x37, 0x60, 0x8a, 0x7f, 0xa3, 0x99 } };
static constexpr GUID guid_preferences_page = { 0x29c963a2, 0x905c, 0x471b, { 0xbc, 0x81, 0xc8, 0xbb, 0x10, 0xb5, 0x3a, 0x68 } };

cfg_string cfg_post_format(guid_cfg_post_format, default_post_format);
cfg_bool cfg_attach_art(guid_cfg_attach_art, default_attach_art);
cfg_uint cfg_art_port(guid_cfg_art_port, default_art_port);
cfg_uint cfg_art_max_edge(guid_cfg_art_max_edge, default_art_max_edge);

namespace
{
	// Windows controls want CRLF line breaks.
	std::string to_crlf(const char* text)
	{
		std::string out;
		for (; *text; text++) {
			if (*text == '\n') out += '\r';
			out += *text;
		}
		return out;
	}

	// Plain Win32 dialog; the SDK's preferences_page_impl<> helper needs WTL.
	// Also listens to playback so the preview follows the playing track.
	class preferences_instance : public preferences_page_instance, private play_callback_impl_base
	{
	public:
		preferences_instance(HWND parent, preferences_page_callback::ptr callback)
			: play_callback_impl_base(flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_dynamic_info_track | flag_on_playback_edited), m_callback(callback)
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
			cfg_post_format = read_format().c_str();
			cfg_attach_art = IsDlgButtonChecked(m_wnd, IDC_ATTACH_ART) == BST_CHECKED;
			cfg_art_port = read_number(IDC_ART_PORT, 1, 65535, (unsigned)cfg_art_port.get());
			cfg_art_max_edge = read_number(IDC_ART_MAX_EDGE, min_art_max_edge, max_art_max_edge, (unsigned)cfg_art_max_edge.get());
			// Show the values as clamped.
			show(cfg_post_format.get().c_str(), cfg_attach_art, (unsigned)cfg_art_port.get(), (unsigned)cfg_art_max_edge.get());
			m_callback->on_state_changed();
		}

		void reset() override
		{
			show(default_post_format, default_attach_art, default_art_port, default_art_max_edge);
			m_callback->on_state_changed();
		}

	private:
		static INT_PTR CALLBACK dialog_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
		{
			if (msg == WM_INITDIALOG) {
				auto self = reinterpret_cast<preferences_instance*>(lp);
				SetWindowLongPtr(wnd, DWLP_USER, lp);
				self->m_wnd = wnd;
				self->show(cfg_post_format.get().c_str(), cfg_attach_art, (unsigned)cfg_art_port.get(), (unsigned)cfg_art_max_edge.get());
				return FALSE;
			}
			auto self = reinterpret_cast<preferences_instance*>(GetWindowLongPtr(wnd, DWLP_USER));
			if (self && msg == WM_DESTROY) {
				self->m_wnd = nullptr;
			}
			if (self && msg == WM_COMMAND && !self->m_showing) {
				switch (LOWORD(wp)) {
				case IDC_POST_FORMAT:
					if (HIWORD(wp) == EN_CHANGE) {
						self->update_preview();
						self->m_callback->on_state_changed();
					}
					break;
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

		void on_playback_new_track(metadb_handle_ptr) override { update_preview(); }
		void on_playback_stop(play_control::t_stop_reason reason) override
		{
			// A new track follows right away.
			if (reason != play_control::stop_reason_starting_another) update_preview();
		}
		void on_playback_dynamic_info_track(const file_info&) override { update_preview(); }
		void on_playback_edited(metadb_handle_ptr) override { update_preview(); }

		void show(const char* format, bool attach, unsigned port, unsigned max_edge)
		{
			m_showing = true;
			uSetDlgItemText(m_wnd, IDC_POST_FORMAT, to_crlf(format).c_str());
			CheckDlgButton(m_wnd, IDC_ATTACH_ART, attach ? BST_CHECKED : BST_UNCHECKED);
			SetDlgItemInt(m_wnd, IDC_ART_PORT, port, FALSE);
			SetDlgItemInt(m_wnd, IDC_ART_MAX_EDGE, max_edge, FALSE);
			update_enabled();
			update_preview();
			m_showing = false;
		}

		void update_enabled()
		{
			const BOOL attach = IsDlgButtonChecked(m_wnd, IDC_ATTACH_ART) == BST_CHECKED;
			EnableWindow(GetDlgItem(m_wnd, IDC_ART_PORT), attach);
			EnableWindow(GetDlgItem(m_wnd, IDC_ART_MAX_EDGE), attach);
		}

		void update_preview()
		{
			if (!m_wnd) return;
			std::string text;
			if (!Kokura_Asahi::format_post(read_format().c_str(), text)) text = "(play something to see the preview)";
			uSetDlgItemText(m_wnd, IDC_POST_PREVIEW, to_crlf(text.c_str()).c_str());
		}

		// The format as stored: LF line breaks.
		std::string read_format()
		{
			pfc::string8 text;
			uGetDlgItemText(m_wnd, IDC_POST_FORMAT, text);
			std::string format = text.c_str();
			format.erase(std::remove(format.begin(), format.end(), '\r'), format.end());
			return format;
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
			return read_format() != cfg_post_format.get().c_str()
				|| (IsDlgButtonChecked(m_wnd, IDC_ATTACH_ART) == BST_CHECKED) != cfg_attach_art
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
