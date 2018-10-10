#include <iostream>
#include <cassert>
#include <typeinfo>

#include "threading.h"
#include "corand.h"
#include "colog.h"

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
				// colog << x;
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
	std::cout << sumOfData << " gets return " << sumOfReturns <<std::endl;
	assert(sumOfReturns == sumOfData);
	// one shot test
	/* 
	auto ret = pool.submit(
		std::function<int(void)>([&]{
			a = corand.UInt(100);
			return a;
		})
	);
	int retVal = ret.get();
	colog << retVal;
	colog << a;
	pool.close();
	assert(retVal == a);
	*/
	colog << "Safely exit...";
	return 0;
}
