#include <iostream>
#include <cassert>
#include <typeinfo>
#include <thread>

#include "threading.h"
#include "corand.h"
#include "colog.h"

using namespace coplus;

constexpr int testSize = 100;

// Assertion
// for block A{}
// ToFiber(A)
// ToFiber(ret)
// happens in the same thread


int main(void){
	ThreadPool pool;
	bool status = false;
	std::future<int> future;

	tgo(pool, future)(std::bind([&](int a)-> int{
		colog << "In Loop Visit";
		int count = 100;
		while(count --)
			yield;
		colog << (1,2);
		colog << "Out Loop Visit";
		status = true;
		return a;
	}, 7));
	colog << future.get();
	while(!status);

	pool.close();
	pool.report(); // left some jobs that spawned from main jobs
	colog << "Safely exit...";
	return 0;
}

