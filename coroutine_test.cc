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
	bool status = false;
	go(pool)(std::function<void(void)>([&](){
		colog << "In Loop Visit";
		int count = 100;
		while(count --)
			yield;
			// yield(*FiberBase::ret_routine());
		colog << "Out Loop Visit";
		status = true;
		return;
	}));
	while(!status);

	pool.close();
	pool.report(); // left some jobs that spawned from main jobs
	colog << "Safely exit...";
	return 0;
}

