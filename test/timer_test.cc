#include <iostream>
#include <cassert>
#include <typeinfo>
#include <thread>
#include <cassert>

#include "coplus/corand.h"
#include "coplus/colog.h"
#include "coplus/coroutine.h"
#include "coplus/async_timer.h"

using namespace coplus;


// proper example with no blocking function
int coplus::main(int argc, char** argv){
	colog << "this is main()";
	// computing here to wait for main scheduler ready
	await(10); // 10 seconds
	colog << "exit main()...";
	return 0;
}
