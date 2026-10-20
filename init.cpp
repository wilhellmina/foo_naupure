#include "stdafx.h"
#include "art_server.h"

class myinitquit : public initquit {
public:
	void on_init() {
	}
	void on_quit() {
		art_server::shutdown();
	}
};
static initquit_factory_t<myinitquit> g_myinitquit_factory;