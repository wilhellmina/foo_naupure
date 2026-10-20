#pragma once
#include <functional>
#include <string>

// One-shot HTTP server on 127.0.0.1 that hands the album art to the userscript.
// Plain Winsock, no foobar2000 SDK dependency.
namespace art_server
{
	typedef std::function<void(const char*)> log_t;

	// Serves `body` once at GET /art, then closes. Also closes after `timeout_ms` without a valid request.
	// A server still running from a previous call is stopped first.
	// Returns false (with the reason in `error`) when the port can't be bound.
	bool start(std::string body, std::string content_type, unsigned short port, unsigned timeout_ms, log_t log, std::string& error);

	// Stops the running server, if any.
	void stop();

	// Stops the server and refuses further start() calls. Call when the app quits.
	void shutdown();
}
