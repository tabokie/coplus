#pragma once

#if defined(__WIN32) || defined(__WIN64) || defined(_MSC_VER)
#define WIN_PORT
#include "Windows.h"
#endif

namespace coplus {

// Type Declare //
#ifdef WIN_PORT
using RawFiber = LPVOID;
const RawFiber NilFiber = NULL;
#else
// not implemented
#endif

// Function Declare //
RawFiber CurrentFiber(void);
RawFiber NewFiber(void (__stdcall*)(void*), void*);
void ToFiber(RawFiber);

// Function Implement //
#ifdef WIN_PORT
RawFiber CurrentFiber(void){
	return GetCurrentFiber();
}
RawFiber NewFiber(VOID (__stdcall *f)(LPVOID) , void* p){
	return CreateFiber(0, f, p);
}
void ToFiber(RawFiber f){
	SwitchToFiber(f);
}
#else
// not implemented
#endif


}

