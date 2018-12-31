#include "coplus/socket.h"
#include "coplus/cotimer.h"

#include "coplus/protocol.h"

#include <string>
#include <sstream>
#include <cassert>
#include <iostream>
using namespace coplus;

int main(int argc, char** argv){
	Server server("myServer");
	// apply new socket
	SOCKET socket;
	std::string addr;
	if(argc >= 3) {
		addr = argv[1];
		addr += ":";
		addr += argv[2];
		socket = Server::NewSocket(argv[1], argv[2]);
	}
	else if(argc == 2) {
		addr = argv[1];
		addr += ":";
		addr += Server::kDefaultPort;
		socket = Server::NewSocket(argv[1]);
	}
	else {
		addr = "127.0.0.1";
		addr += ":";
		addr += Server::kDefaultPort;
		socket = Server::NewSocket("127.0.0.1");
	}
	assert(socket != INVALID_SOCKET); 

	// read input and close server
	std::atomic<bool> close = false;
	std::thread daemon = std::thread([&]{
		std::string str;
		std::string terminator = "exit";
		while(std::cin >> str) {
			if(str == terminator) {
				colog << "receive terminator";
				server.Close();
				break;
			}
		}
	});
	// serve single socket
	std::cout << "server listening on " + addr << std::endl;
	server.Serve(
		socket, 
		// response when receiving client message
		[&](SOCKET sender, const char* buf, size_t len, std::vector<std::string>& ret) {
			static std::ostringstream str_helper;
			static bool pending;
			static std::string pendingStr;
			static auto parse = [&](std::string in, std::string& out)-> int {
				std::string head(Protocol::app_head_len, ' ');
				memcpy((void*)head.c_str(), (void*)&Protocol::app_head, Protocol::app_head_len);
				std::string tail(Protocol::app_terminator_len, ' ');
				memcpy((void*)tail.c_str(), (void*)&Protocol::app_terminator, Protocol::app_terminator_len);
				int h = in.find(head);
				int t = in.find(tail);
				bool has_head = (h != std::string::npos);
				bool has_end = (t != std::string::npos);
				if(has_head && !pending) {
					if(has_end) {
						str_helper.str("");
						str_helper.clear();
						out = Protocol::format_message(
							str_helper,
							in.substr(
								h + Protocol::app_head_len, 
								t - h - Protocol::app_head_len
							),
							Protocol::kGetContent
						);
						colog << str_helper.str();
					} else {
						pending = true;
						pendingStr = in.substr(
							h + Protocol::app_head_len, 
							in.size() - Protocol::app_terminator_len - h - Protocol::app_head_len
						);
					}
				} else if(pending) {
					if(has_end) { // finish pending messaeg
						pendingStr += in.substr(0, t);
						str_helper.str("");
						str_helper.clear();
						out = Protocol::format_message(str_helper, pendingStr, Protocol::kGetContent);
						colog << str_helper.str();
						pending = false;
						pendingStr.clear();
					} else {
						pendingStr += in;
					}
				}
				return pending ? -1 : t + Protocol::app_terminator_len;
			};
			static auto dispatch = [&](std::string content) -> std::string {
				std::string markers = "time|name|list|relay";
				// search for markers
				if(content.size() < 4) return "invalid request";
				if(content.compare(0,4, markers, 0, 4) == 0) {
					return Protocol::pickle_message(Protocol::kResponse, cotimer.describe());
				}
				else if(content.compare(0,4, markers, 5, 4) == 0) {
					return Protocol::pickle_message(Protocol::kResponse, server.name());
				}
				else if(content.compare(0,4, markers, 10, 4) == 0) {
					std::vector<std::string> header;
					std::vector<std::string> entries;
					server.OutputDatabase(header, entries);
					return Protocol::pickle_message(Protocol::kResponse, header, entries);
				}
				else if(content.size() >= 5 && content.compare(0,5, markers, 15, 5) == 0) { // relay80:...
					int id = atoi(content.c_str() + 5);
					SOCKET target = server.GetClient(id);
					if(target == INVALID_SOCKET) return Protocol::pickle_message(Protocol::kResponse, "invalid relay target");
					int start = content.find(":");
					if(target == sender) { // to avoid buffering
						return Protocol::pickle_message(Protocol::kDirect, Server::GetPeerAddr(sender), content.substr(start + 1));
					}
					else{
						return Protocol::pickle_message(
							Protocol::kResponse, 
							"relay status = " + 
							std::to_string(
								server.Send(
									target, 
									Protocol::pickle_message(Protocol::kDirect, Server::GetPeerAddr(sender), content.substr(start + 1))
								)
							)
						);
					}		
				}
				return "invalid request";
			};
			std::string line(len, ' '); // may be \0
			memcpy((void*)line.c_str(), buf, len);
			std::string content;
			std::string str = "";
			int cur = 0, delta;
			while(true) {
				if( cur >= line.size() || (delta = parse(line.substr(cur), content)) < 0 ) break; // find a pending
				cur += delta;
				str = dispatch(content);
				if(str.size() > 0) ret.push_back(str);
			}
		}
	);
	daemon.join();
	return 0;
}