/*
	Copyright(c) 2021-2023 jvde.github@gmail.com

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include <list>
#include <thread>
#include <mutex>
#include <time.h>

#ifdef HASCURL
#include <curl/curl.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#define SOCKET		int
#define closesocket close
#endif

#ifdef __ANDROID__
#include <netinet/in.h>
#endif

#include "Stream.h"
#include "Common.h"
#include "AIS.h"
#include "Keys.h"
#include "TCP.h"
#include "Utilities.h"
#include "Library/ZIP.h"

namespace IO {

	struct Client {
		~Client() { Close(); }
		
		SOCKET sock = -1;
		std::string msg;
		std::time_t stamp;

		void Close();
		void Start(SOCKET s);
		int Inactive(std::time_t now);
		bool isConnected() { return sock != -1; }

		void Read();
	};

	class TCPServer {
	public:
		~TCPServer();

		bool start(int port);
		bool SendAll(const std::string &m) {
			for (auto& c : client) {
				if (c.isConnected()) {
					if (!Send(c.sock, m.c_str(), m.size())) {
						c.Close();
						std::cerr << "TCP listener: error sending to client, close connection." << std::endl;
						return false;
					}
				}
			}
			return true;
		}
		void setReusePort(bool b) { reuse_port = b; }
		bool setNonBlock(SOCKET sock) {

#ifndef _WIN32
			int r = fcntl(sock, F_GETFL, 0);
			r = fcntl(sock, F_SETFL, r | O_NONBLOCK);

			if (r == -1) return false;
#else
			u_long mode = 1;
			ioctlsocket(sock, FIONBIO, &mode);
#endif
			return true;
		}

	protected:
		SOCKET sock = -1;
		int timeout = 30;
		bool reuse_port = true;

		const int MAX_CONN = 64;
		std::vector<Client> client;

		struct addrinfo* address;
		std::thread run_thread;

		bool stop = false;

		void Run();
		sockaddr_in service;

		bool Send(SOCKET s, const char* data, int len);
		int findFreeClient();

		void acceptClients();
		void readClients();
		virtual void processClients();
		void cleanUp();
		void SleepAndWait();
	};

	class Server : public TCPServer {
	public:
		virtual void Request(Client& c, const std::string& msg, bool accept_gzip);

		void Response(Client& c, std::string type, const std::string& content, bool gzip = false);
		void Response(Client& c, std::string type, char* data, int len, bool gzip = false);

	private:
		std::string ret, header;

		void Parse(const std::string& s, std::string& get, bool& accept_gzip);
		void processClients();

		ZIP zip;
	};
}
