// Not using the PCH: winsock2.h has to come before windows.h.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "art_server.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

namespace art_server
{
	namespace
	{
		typedef std::chrono::steady_clock steady;

		struct instance
		{
			SOCKET listener = INVALID_SOCKET;
			std::atomic<bool> stop_requested{ false };
			std::thread worker;
		};

		std::mutex g_lock;
		std::unique_ptr<instance> g_instance;
		bool g_wsa_started = false;
		bool g_shut_down = false;

		std::string wsa_error(const char* what)
		{
			return std::string(what) + " failed (WSA error " + std::to_string(WSAGetLastError()) + ")";
		}

		// Waits until `s` is readable. Returns false on deadline, stop request or error.
		bool wait_readable(SOCKET s, steady::time_point deadline, std::atomic<bool> const& stop)
		{
			while (!stop) {
				auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - steady::now()).count();
				if (left <= 0) return false;
				// Short slices so that stop() doesn't have to wait long.
				timeval tv = { 0, (long)std::min<long long>(left, 100) * 1000 };
				fd_set set;
				FD_ZERO(&set);
				FD_SET(s, &set);
				int ret = select(0, &set, nullptr, nullptr, &tv);
				if (ret > 0) return true;
				if (ret < 0) return false;
			}
			return false;
		}

		bool send_all(SOCKET s, const char* data, size_t size)
		{
			while (size > 0) {
				int sent = send(s, data, (int)std::min<size_t>(size, 1 << 20), 0);
				if (sent <= 0) return false;
				data += sent;
				size -= sent;
			}
			return true;
		}

		bool iequals(std::string_view a, std::string_view b)
		{
			return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
				return tolower((unsigned char)x) == tolower((unsigned char)y);
			});
		}

		std::string_view trim(std::string_view s)
		{
			while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
			while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
			return s;
		}

		// Only our own names are accepted, so that a page served from another host name
		// resolving to 127.0.0.1 (DNS rebinding) can't read the image.
		bool host_allowed(std::string_view host, unsigned short port)
		{
			const std::string suffix = ":" + std::to_string(port);
			for (const char* name : { "127.0.0.1", "localhost" }) {
				if (iequals(host, name + suffix)) return true;
				if (port == 80 && iequals(host, name)) return true;
			}
			return false;
		}

		struct request
		{
			std::string_view method, target, host;
			bool has_host = false;
		};

		bool parse_request(std::string_view head, request& out)
		{
			size_t eol = head.find("\r\n");
			std::string_view line = head.substr(0, eol);
			size_t sp1 = line.find(' ');
			size_t sp2 = sp1 == line.npos ? line.npos : line.find(' ', sp1 + 1);
			if (sp2 == line.npos) return false;
			out.method = line.substr(0, sp1);
			out.target = line.substr(sp1 + 1, sp2 - sp1 - 1);

			size_t pos = eol == head.npos ? head.size() : eol + 2;
			while (pos < head.size()) {
				size_t end = std::min(head.find("\r\n", pos), head.size());
				std::string_view field = head.substr(pos, end - pos);
				size_t colon = field.find(':');
				if (colon != field.npos && iequals(trim(field.substr(0, colon)), "host")) {
					if (out.has_host) return false;
					out.host = trim(field.substr(colon + 1));
					out.has_host = true;
				}
				pos = end + 2;
			}
			return true;
		}

		std::string response_head(const char* status, std::string_view type, size_t length, const char* extra = "")
		{
			std::string head = std::string("HTTP/1.1 ") + status + "\r\n";
			head += "Content-Type: ";
			head += type;
			head += "\r\nContent-Length: " + std::to_string(length) + "\r\n";
			head += "Cache-Control: no-store\r\nConnection: close\r\n";
			head += extra;
			head += "\r\n";
			return head;
		}

		enum class outcome { no_request, rejected, delivered };

		// Answers one request.
		outcome serve_client(SOCKET client, instance& self, std::string const& body, std::string const& type, unsigned short port, log_t const& log)
		{
			std::string head;
			size_t end;
			const auto deadline = steady::now() + std::chrono::seconds(3);
			while ((end = head.find("\r\n\r\n")) == head.npos) {
				if (head.size() > 8192 || !wait_readable(client, deadline, self.stop_requested)) return outcome::no_request;
				char buf[2048];
				int got = recv(client, buf, sizeof buf, 0);
				if (got <= 0) return outcome::no_request;
				head.append(buf, got);
			}
			head.resize(end);

			request req;
			const char* status = nullptr;
			const char* extra = "";
			if (!parse_request(head, req)) status = "400 Bad Request";
			else if (!req.has_host || !host_allowed(req.host, port)) status = "403 Forbidden";
			else if (req.method != "GET") {
				status = "405 Method Not Allowed";
				extra = "Allow: GET\r\n";
			}
			else if (req.target != "/art") status = "404 Not Found";

			if (status) {
				if (log) log((std::string("art server: rejected request (") + status + ")").c_str());
				std::string response = response_head(status, "text/plain", strlen(status), extra) + status;
				send_all(client, response.data(), response.size());
				return outcome::rejected;
			}
			std::string response = response_head("200 OK", type, body.size());
			bool sent = send_all(client, response.data(), response.size()) && send_all(client, body.data(), body.size());
			return sent ? outcome::delivered : outcome::no_request;
		}

		void reset_client(SOCKET client)
		{
			linger abort_close = { 1, 0 };
			setsockopt(client, SOL_SOCKET, SO_LINGER, (const char*)&abort_close, sizeof abort_close);
			closesocket(client);
		}

		// Sends FIN, then waits for the client to hang up so that it gets the whole response
		// rather than a reset. Resets the connection if the client doesn't close.
		void close_client(SOCKET client, std::atomic<bool> const& stop)
		{
			::shutdown(client, SD_SEND);
			const auto deadline = steady::now() + std::chrono::seconds(2);
			bool peer_closed = false;
			while (!peer_closed && wait_readable(client, deadline, stop)) {
				char buf[1024];
				int got = recv(client, buf, sizeof buf, 0);
				if (got == 0) peer_closed = true;
				else if (got < 0) break;
			}
			if (peer_closed) closesocket(client);
			else reset_client(client);
		}

		void run(instance& self, std::string body, std::string type, unsigned short port, unsigned timeout_ms, log_t log)
		{
			const auto deadline = steady::now() + std::chrono::milliseconds(timeout_ms);
			SOCKET delivered_to = INVALID_SOCKET;
			while (wait_readable(self.listener, deadline, self.stop_requested)) {
				SOCKET client = accept(self.listener, nullptr, nullptr);
				if (client == INVALID_SOCKET) continue;
				outcome result = serve_client(client, self, body, type, port, log);
				if (result == outcome::delivered) {
					delivered_to = client;
					break;
				}
				if (result == outcome::rejected) close_client(client, self.stop_requested);
				else reset_client(client);
			}
			// Stop listening first; the userscript already has what it needs.
			closesocket(self.listener);
			self.listener = INVALID_SOCKET;

			if (delivered_to != INVALID_SOCKET) {
				close_client(delivered_to, self.stop_requested);
				if (log) log("art server: artwork sent");
			}
			else if (!self.stop_requested) {
				if (log) log("art server: no request for the artwork in time, closed (is the userscript installed?)");
			}
		}

		void stop_locked()
		{
			if (!g_instance) return;
			g_instance->stop_requested = true;
			if (g_instance->worker.joinable()) g_instance->worker.join();
			g_instance.reset();
		}
	}

	bool start(std::string body, std::string content_type, unsigned short port, unsigned timeout_ms, log_t log, std::string& error)
	{
		std::lock_guard<std::mutex> guard(g_lock);
		if (g_shut_down) {
			error = "shutting down";
			return false;
		}
		stop_locked();

		if (!g_wsa_started) {
			WSADATA wsa;
			int ret = WSAStartup(MAKEWORD(2, 2), &wsa);
			if (ret != 0) {
				error = "WSAStartup failed (" + std::to_string(ret) + ")";
				return false;
			}
			g_wsa_started = true;
		}

		SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET) {
			error = wsa_error("socket");
			return false;
		}
		// Keep other processes from binding the same port next to us.
		BOOL exclusive = TRUE;
		setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&exclusive, sizeof exclusive);

		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		if (bind(s, (const sockaddr*)&addr, sizeof addr) == SOCKET_ERROR) {
			error = wsa_error("bind");
			closesocket(s);
			return false;
		}
		if (listen(s, 4) == SOCKET_ERROR) {
			error = wsa_error("listen");
			closesocket(s);
			return false;
		}

		auto inst = std::make_unique<instance>();
		inst->listener = s;
		try {
			inst->worker = std::thread(run, std::ref(*inst), std::move(body), std::move(content_type), port, timeout_ms, std::move(log));
		}
		catch (std::exception const& e) {
			error = std::string("thread creation failed: ") + e.what();
			closesocket(s);
			return false;
		}
		g_instance = std::move(inst);
		return true;
	}

	void stop()
	{
		std::lock_guard<std::mutex> guard(g_lock);
		stop_locked();
	}

	void shutdown()
	{
		std::lock_guard<std::mutex> guard(g_lock);
		g_shut_down = true;
		stop_locked();
		if (g_wsa_started) {
			WSACleanup();
			g_wsa_started = false;
		}
	}
}
