#include <iostream>
#include <cassert>
#include <typeinfo>
#include <thread>
#include <cassert>

#include "coplus/corand.h"
#include "coplus/colog.h"
#include "coplus/coroutine.h"

using namespace coplus;

constexpr int testSize = 100;

// main entry signature
// int coplus::main(int, char**);
// function block will be submit as a simple task, 
// then main thread is itself turned into a scheduler.

// in this case, a scheduler will block at mission main for trace.get()
/*
int coplus::main(int argc, char** argv){
  // Use Case with returned function
	auto trace = go(std::bind([&](int ret)-> int{
	  	colog << "In Loop Visit";
	  	int count = 100;
	  	while(count --){
	  		auto trace = await([]{int doCompute = 1000; while(doCompute--); return 7;});
	  		// colog << trace.get();
	  	}
	  	colog << "Out Loop Visit";
	  	return ret;
	  }, 42));
  colog << trace.get(); // expect 42
	colog << "exit main()...";
	return 0;
}
*/

int compute(int amount){
	int ret = 0;
	float base = 100;
	while(amount --){
		base *= corand.UFloat() * 2;
		ret += base;
	}
	return ret;
}

// proper example with no blocking function
int coplus::main(int argc, char** argv){
	colog << "this is main()";
	// computing here to wait for main scheduler ready
	colog << compute(100000);
	int count = 1000;
	int total = 0;
	while(count --){
	  auto trace = await([]{return compute(100);});
	  total += trace.get();
	}
	colog << total;
	colog << "exit main()...";
	return 0;
}
