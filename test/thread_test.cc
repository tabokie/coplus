#include <iostream>
#include <cassert>
#include <typeinfo>
#include <thread>

#include "coplus/thread_pool.h"
#include "coplus/corand.h"
#include "coplus/colog.h"

using namespace coplus;

constexpr int testSize = 100;

int main(void){
	ThreadPool pool;
	int commonData[testSize];
	int sumOfData = 0;
	for(int i = 0; i < testSize; i++){
		// commonData[i] = i;
		commonData[i] = corand.UInt(100);
		sumOfData += commonData[i];
	}
	std::vector<std::future<int>> futures;
	for(int i = 0; i < testSize; i++){
		// colog << (std::string("new task: ") + std::to_string(commonData[i]));
		futures.push_back(std::move(pool.submit(
			std::function<int(void)>([&, x = commonData[i]]{
				// spawn one more thread
				pool.submit(std::function<int(void)>([]{
					int count = 1000; // simulate cpu-bound job
					while(count --)
						std::this_thread::yield();
					return 0;
				}));
				return x;
			})
		)));
		assert(futures[i].valid());
	}
	int sumOfReturns = 0;
	for(int i = 0; i < testSize; i++){
		assert(futures[i].valid());
		int ret = futures[i].get();
		// colog << (std::string("new return: ") + std::to_string(ret));
		sumOfReturns += ret;
	}
	pool.close();
	pool.report(); // left some jobs that spawned from main jobs
	std::cout << sumOfData << " gets return " << sumOfReturns <<std::endl;
	assert(sumOfReturns == sumOfData);
	colog << "Safely exit...";
	return 0;
}

