#include "coplus/socket.h"
#include "coplus/cotimer.h"

#include <string>
#include <cassert>
#include <iostream>
using namespace coplus;

int main(int argc, char** argv){
	Server server("myServer");
	// apply new socket
	SOCKET socket;
	if(argc >= 2) {
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
			}
		}
	});
	// serve single socket
	server.Serve(
		socket, 
		// response when receiving client message
		[&](SOCKET sender, std::string message)-> std::string {
			colog.put_batch("receive from client: ", message );
			std::string markers = "time|name|list|relay";
			int start;
			for(start = 0; start < message.length() && message[start] == ' '; start ++); // eliminate leading spaces
			if(start == message.length())return "empty message";
			// search for markers
			if(message.compare(start,4, markers, 0, 4) == 0) {
				return cotimer.describe();
			}
			else if(message.compare(start,4, markers, 5, 4) == 0) {
				return server.name();
			}
			else if(message.compare(start,4, markers, 10, 4) == 0) {
				return server.database_string();
			}
			else if(message.compare(start,5, markers, 15, 5) == 0) { // relay80:...
				int id = atoi(message.c_str() + 5);
				SOCKET target = server.GetClient(id);
				if(target == INVALID_SOCKET) return "invalid relay target";
				start = message.find(":");
				if(target == sender) { // to avoid buffering
					return std::string("relay back: ") + message.substr(start + 1);
				}
				else{
					return std::string("relay status: ") + std::to_string(server.Relay(sender, target, message.substr(start + 1)));
				}		
			}
			return "invalid request";
		}
	);
	daemon.join();
	return 0;
}