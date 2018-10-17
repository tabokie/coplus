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
	pool.go(std::function<void(RawFiber*)>([&](RawFiber* ret){
		colog << "In Loop Visit";
		int count = 100;
		while(count --)
			yield(*ret);
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

