#pragma once

#if defined(__WIN32) || defined(__WIN64) || defined(_MSC_VER)
#define WIN_PORT
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Windows.h"
#elif defined(_linux)
#define POSIX_PORT
#include "ucontext.h"
#endif