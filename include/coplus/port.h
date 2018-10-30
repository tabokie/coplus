#pragma once

#if defined(__WIN32) || defined(__WIN64) || defined(_MSC_VER)
#define WIN_PORT
#elif defined(_linux)
#define POSIX_PORT
#endif