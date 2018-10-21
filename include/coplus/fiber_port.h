#pragma once

#include "colog.h"

#if defined(__WIN32) || defined(__WIN64) || defined(_MSC_VER)
#define WIN_PORT
#include "Windows.h"
#else
#define POSIX_PORT
#include "ucontext.h"
#endif

namespace coplus {

// Type Declare //
#ifdef WIN_PORT
using RawFiber = LPVOID;
const RawFiber NilFiber = NULL;
#elif defined(POSIX_PORT)
using RawFiber = ucontext_t*;
const RawFiber NilFiber = NULL;
#endif

// Function Declare //
void InitEnv(void);
RawFiber CurrentFiber(void);
RawFiber NewFiber(void (__stdcall*)(void*), void*);
void ToFiber(RawFiber target = NilFiber);

}

