#include <iostream>
#include <cassert>
#include <typeinfo>
#include <thread>

#include "corand.h"
#include "colog.h"
#include "coroutine.h"

using namespace coplus;

constexpr int testSize = 100;

int main(void){
  // Use Case with returned function
	auto trace = go(std::bind([&](int ret)-> int{
	  	colog << "In Loop Visit";
	  	int count = 100;
	  	while(count --)
	  		yield();
	  	colog << "Out Loop Visit";
	  	return ret;
	  }, 42));
  colog << trace.get(); // expect 42

	colog << "exit main()...";
	return 0;
}

