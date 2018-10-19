#include <iostream>
#include <cassert>
#include <typeinfo>
#include <thread>

#include "threading.h"
#include "corand.h"
#include "colog.h"

using namespace coplus;

constexpr int testSize = 100;

int main(void){
	ThreadPool pool;
  // User Case with returned function
	auto trace = pool.go(std::bind([&](int ret)-> int{
	  	colog << "In Loop Visit";
	  	int count = 100;
	  	while(count --)
	  		yield();
	  	colog << "Out Loop Visit";
	  	return ret;
	  }, 42));
  colog << trace.get(); // expect 42

	pool.close();
	pool.report(); // left some jobs that spawned from main jobs
	colog << "Safely exit...";
	return 0;
}

