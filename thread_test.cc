// #include <gtest/gtest.h>

#include "threading.h"
#include "corand.h"
#include "colog.h"
#include <iostream>
#include <cassert>

using namespace coplus;



int main(void){
	ThreadPool pool;
	int a = -1;
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
	return 0;
}
