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
	if(argc >= 3) {
		socket = Server::NewSocket(argv[1], argv[2]);
	}
	else if(argc == 2) {
		socket = Server::NewSocket(argv[1]);
	}
	else {
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
	server.Serve(
		socket, 
		// response when receiving client message
		[&](SOCKET sender, const char* buf, size_t len)-> std::string {
			static std::ostringstream str_helper;
			static bool pending;
			static std::string pendingStr;
			std::string line(len, ' '); // may be \0
			memcpy((void*)line.c_str(), buf, len);
			std::string content = "";
			bool has_head = (line.size() > Protocol::minimum_frame) && 
				(*((unsigned int*)line.c_str()) == Protocol::app_head);
			bool has_end = (line.size() > Protocol::minimum_frame) && 
				(*((unsigned int*)(line.c_str() + line.size() - Protocol::app_terminator_len)) == Protocol::app_terminator);
			if(has_head) {
				pending = false; // discard pending message
				if(has_end) {
					str_helper.str("");
					str_helper.clear();
					content = Protocol::format_message(
						str_helper,
						line.substr(
							Protocol::app_head_len, 
							line.size() - Protocol::app_head_len - Protocol::app_terminator_len
						),
						Protocol::kGetContent
					);
					colog << str_helper.str();
				} else {
					pending = true;
					pendingStr = line.substr(
						Protocol::app_head_len,
						line.size() - Protocol::app_head_len - Protocol::app_terminator_len
					);
				}
			} else if(pending) {
				if(has_end) { // finish pending messaeg
					pendingStr += line.substr(0, line.size() - Protocol::app_terminator_len);
					str_helper.str("");
					str_helper.clear();
					content = Protocol::format_message(str_helper, pendingStr, Protocol::kGetContent);
					colog << str_helper.str();
					pending = false;
					pendingStr.clear();
				} else {
					pendingStr += line;
				}
			}
			if(pending || content.size() == 0) return "";
			std::string markers = "time|name|list|relay";
			// search for markers
			if(content.compare(0,4, markers, 0, 4) == 0) {
				return Protocol::pickle_message(Protocol::kRequest, cotimer.describe());
			}
			else if(content.compare(0,4, markers, 5, 4) == 0) {
				return Protocol::pickle_message(Protocol::kRequest, server.name());
			}
			else if(content.compare(0,4, markers, 10, 4) == 0) {
				std::vector<std::string> header;
				std::vector<std::string> entries;
				server.OutputDatabase(header, entries);
				return Protocol::pickle_message(Protocol::kRequest, header, entries);
			}
			else if(content.compare(0,5, markers, 15, 5) == 0) { // relay80:...
				int id = atoi(content.c_str() + 5);
				SOCKET target = server.GetClient(id);
				if(target == INVALID_SOCKET) return Protocol::pickle_message(Protocol::kRequest, "invalid relay target");
				int start = content.find(":");
				if(target == sender) { // to avoid buffering
					return Protocol::pickle_message(Protocol::kDirect, content.substr(start + 1));
				}
				else{
					return Protocol::pickle_message(
						Protocol::kRequest, 
						"relay status = " + 
						std::to_string(
							server.Send(
								target, 
								Protocol::pickle_message(Protocol::kRequest, content.substr(start + 1))
							)
						)
					);
				}		
			}
			return "invalid request";
		}
	);
	daemon.join();
	return 0;
}