#include "stdafx.h"

class PlaybackCallback : public play_callback_static
{
public:
	unsigned get_flags() override ;
	void on_playback_starting(play_control::t_track_command cmd, bool paused) override
	{ // ignore
	}
	void on_playback_new_track(metadb_handle_ptr track) override;
	void on_playback_stop(play_control::t_stop_reason reason) override {};
	void on_playback_seek(double time) override {};
	void on_playback_pause(bool state) override {};
	void on_playback_edited(metadb_handle_ptr track) override {};
	void on_playback_dynamic_info(const file_info& info) override
	{ // ignore
	}
	void on_playback_dynamic_info_track(const file_info& info) override {};
	void on_playback_time(double time) override {};
	void on_volume_change(float p_new_val) override
	{ // ignore
	}
private:
	void on_playback_changed(metadb_handle_ptr track = metadb_handle_ptr()) {};
	bool needPresenceRefresh_ = false;
};

unsigned PlaybackCallback::get_flags()
{
	return flag_on_playback_all;
}

void PlaybackCallback::on_playback_new_track(metadb_handle_ptr track)
{
	file_info_impl info;
	track->get_info(info);
	
	console::print(info.meta_get("TITLE",0));
}

play_callback_static_factory_t<PlaybackCallback> playbackCallback;
