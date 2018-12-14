#pragma once

#include "port.h" // WIN_PORT
#include "colog.h" // colog

#include <iostream>
#include <vector>
#include <future>
#include <thread>

#ifdef POSIX_PORT
#error "posix socket not implemented"
#endif

// assume WIN_PORT
// #define WIN32_LEAN_AND_MEAN

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")
// #pragma comment (lib, "AdvApi32.lib")


namespace coplus {

// constant for buffer
#define DEFAULT_BUFLEN 512
// fallback port
#define DEFAULT_PORT "27015"

// static setup for windows platform
struct WSAProof {
	friend class Socket;
 private:
	WSADATA wsa_data_;
	int startup_result_;
 public:
 	WSAProof(){
		startup_result_ = WSAStartup(MAKEWORD(2,2), &wsa_data_);
	}
	~WSAProof(){
		WSACleanup();
	}
	bool ok(void) const {
		return startup_result_ == 0;
	}
};

class Server {
	static const WSAProof wsa_;
	struct ClientSocketInfo {
		SOCKET handle = INVALID_SOCKET;
		std::string addr; // ip:port form
		bool status = true;
		ClientSocketInfo(SOCKET sck, std::string adr): addr(adr), handle(sck) { }
		std::string toString(int id) {
			return std::string("{") + 
				"id:" + std::to_string(id) +
				", address:" + addr +
				", status:" + (status? "on" : "off") +
				"}";
		}
	};
	std::vector<ClientSocketInfo> database_;
	std::string name_;
 public:
 	Server(std::string n): name_(n) { }
 	std::string name(void) {
 		return name_;
 	}
 	std::string listToString(void) {
 		std::string ret;
 		int id = 0;
 		for(auto& client: database_) {
 			if(id == 0)
 				ret = client.toString(id);
 			else
 				ret = ret + "," + client.toString(id);
 			id ++;
 		}
 		return ret;
 	}
 	SOCKET lookupClient(int id) {
 		if(id < 0 || id >= database_.size()) {
 			return INVALID_SOCKET;
 		}
 		return database_[id].handle;
 	}
 	void closeClient(int id) {
 		if(id < 0 || id >= database_.size()) return;
 		database_[id].status = false;
 		closesocket(database_[id].handle);
 	}
 	bool relay(SOCKET sender, SOCKET target, std::string message) { // relay
 		message = GetPeerAddr(sender) + " send: " + message;
 		int iSendResult = send(target, message.c_str(), message.length(), 0);
 		if(iSendResult == SOCKET_ERROR) return false;
 		return true;
 	}
 	bool reply(SOCKET sender, std::string message) {
 		message = GetSocketAddr(sender) + " reply: " + message;
 		int iSendResult = send(sender, message.c_str(), message.length(), 0);
 		if(iSendResult == SOCKET_ERROR) return false;
 		return true;
 	}
 	template <typename ResponsFunction, typename CallbackFunction>
 	bool serve(SOCKET server, ResponsFunction&& response, CallbackFunction&& callback) { // bool callback(bool)
 		SOCKET client;
 		// std::vector<std::future<bool>> rets;
 		std::vector<std::thread> threads;
    if (listen(server, SOMAXCONN) == SOCKET_ERROR) { // open listen port
        colog.error("listen failed with error: ", WSAGetLastError());
        closesocket(server);
        return false;
    }
 		do {
 			client = accept(server, NULL, NULL); // block here
 			if(client == INVALID_SOCKET) {
 				if(callback()) // not due to voluntary closing
 					colog.error("error when accepting new connection");
 				break; // fatal error
 			}
 			// add client message to database
 			database_.push_back(ClientSocketInfo(client, GetPeerAddr(client))) ;
 			int id = database_.size() - 1;
 			// create a new thread
 			threads.push_back(std::thread(std::move([&]()-> bool{
				char recvbuf[DEFAULT_BUFLEN];
				int recvbuflen = DEFAULT_BUFLEN;
				do{
 					int iResult = recv(client, recvbuf, recvbuflen, 0);
 					if(iResult > 0) {
 						recvbuf[iResult] = '\0';
 						if(!reply(client, response(client, recvbuf))){
 							colog.error("send error: ", WSAGetLastError() );
 							closeClient(id);
 							return false;
 						}
 					}
 					else if(iResult == 0) { // closed by client
 						break;
 					}
 					else {
 						colog.error("recv error: ", WSAGetLastError());
 						closeClient(id);
 						return false;
 					}	
				} while(callback());
				closeClient(id);
				return true;
			})));
			threads[threads.size()-1].detach();
 		} while(callback()); // a success connection, continue?
 		// join all subthread
		std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join) );
 		closesocket(server);
 		return true;
 	}

 	static SOCKET NewSocket(const char* addr) {
 		return NewSocket(addr, DEFAULT_PORT);
 	}
 	static SOCKET NewSocket(const char* addr, const char* port) {
		if(!wsa_.ok())return INVALID_SOCKET;
		struct addrinfo *result, *ptr, hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Resolve the server address and port
    int iResult = getaddrinfo(addr, port, &hints, &result);
    if ( iResult != 0 ) {
    	colog.error("getaddrinfo failed with error: ", iResult);
    	freeaddrinfo(result);
    	return INVALID_SOCKET;
    }
    SOCKET ret = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    iResult = bind(ret, result->ai_addr, (int)result->ai_addrlen);
    if ( iResult == SOCKET_ERROR) {
    	colog.error("bind failed with error: " , WSAGetLastError());
    	freeaddrinfo(result);
    	return INVALID_SOCKET;
    }
    freeaddrinfo(result);
    return ret;
	}
	static std::string GetSocketAddr(SOCKET socket) {
		sockaddr_in addr;
		memset(&addr,0,sizeof(addr));
		int len = sizeof(addr);
		int ret = getsockname(socket,(sockaddr*)&addr,&len);
		if (ret != 0) {
			colog.error("get sock addr error");
			return "";
		}
		return std::string(inet_ntoa(addr.sin_addr)) + ":" + std::to_string(ntohs(addr.sin_port));
	}
	static std::string GetPeerAddr(SOCKET socket) {
		sockaddr_in addr;
		memset(&addr,0,sizeof(addr));
		int len = sizeof(addr);
		int ret = getpeername(socket,(sockaddr*)&addr,&len);
		if (ret != 0) {
			colog.error("get sock addr error");
			return "";
		}
		char ipAddr[INET_ADDRSTRLEN];
		return std::string(inet_ntop(AF_INET, &addr.sin_addr, ipAddr, sizeof(ipAddr))) +":"+ std::to_string(ntohs(addr.sin_port));
	}

};


}
