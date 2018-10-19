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

// Function Implement //
#ifdef WIN_PORT
void InitEnv(void){
	ConvertThreadToFiber(NULL);
}
RawFiber CurrentFiber(void){
	return GetCurrentFiber();
}
RawFiber NewFiber(void (__stdcall *f)(void*) , void* p){
	return CreateFiber(0, f, p);
}
// Assertion for correctness //
// for coroutine block A{}
// enter: ToFiber(A) and exit: ToFiber(ret_addr) happen in the same thread continuously
// even for nested coroutine, 
// child routine is a new task that executes in another thread or after parent task yields
void ToFiber(RawFiber f){
	thread_local RawFiber trace = NilFiber;
	RawFiber temp = trace;
	trace = CurrentFiber();
	if(f == NilFiber){
		SwitchToFiber(temp); // return
	}
	else{
		SwitchToFiber(f);		
	}
}
#elif defined(POSIX_PORT)
// void InitEnv(void){
// 	// empty
// 	return ;
// }
// RawFiber CurrentFiber(void){
// 	return NilFiber;
// }
// RawFiber NewFiber(void (__stdcall* f)(void*), void* p){
// 	RawFiber ret;
// 	makecontext(&ret, f, p);
// 	return ret;
// }
// void ToFiber(RawFiber f){
// 	RawFiber threadFiber = CurrentFiber();
// 	swapcontext(threadFiber, f); // preserve and update
// }
#endif


}

