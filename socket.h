#pragma once

#include "log.hpp"

#include <iostream>

#if defined(__WIN32) || defined(__WIN64) || defined(_MSC_VER)
#define SOCKET_WIN_IMPL
#else 
#define SOCKET_LINUX_IMPL
#endif

// header file and lib
#ifdef SOCKET_WIN_IMPL
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#endif

// constant for buffer
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#ifdef SOCKET_WIN_IMPL
// class Socket;

struct WSAProof {
	friend class Socket;
 private:
	WSADATA wsa_data_;
	int startup_result_;
	WSAProof(){
		startup_result_ = WSAStartup(MAKEWORD(2,2), &wsa_data_);
	}
	~WSAProof(){
		WSACleanup();
	}
	bool SoundProof(void) const {
		return startup_result_ == 0;
	}
};
class Socket {
	static const WSAProof wsa_;
	SOCKET socket_ = INVALID_SOCKET;
 public:
	static SOCKET NewWinSocket(const char* addr, const char* port) {
		if(!wsa_.SoundProof())return INVALID_SOCKET;
		struct addrinfo *result, *ptr, hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Resolve the server address and port
    int iResult = getaddrinfo(addr, port, &hints, &result);
    if ( iResult != 0 ) {
      std::cout << "getaddrinfo failed with error: " << iResult << std::endl;;
    	return INVALID_SOCKET;
    }
    auto ret = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    freeaddrinfo(result);
    return ret;
	}
	Socket(const char* addr, const char* port): socket_(NewWinSocket(addr, port)) { }
	Socket(const char* addr): socket_(NewWinSocket(addr, DEFAULT_PORT)) { }
};
#elif SOCKET_LINUX_IMPL
class Socket {
	// no implementation yet
};
#endif