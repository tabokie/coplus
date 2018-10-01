#pragma once

#include <thread>
#include <cstdio>
#include <mutex>
#include <iostream>
#include <functional>
#include <string>
#include <atomic>
#include <cstdio>

#include "util.h"

namespace coplus {

namespace {

std::hash<std::thread::id> hasher;

// colored logging
class ThreadInfo{
 public:
	static std::string thread_string(void){
		return std::string("[thread ") + std::to_string(thread_id()) + "]";
	}
	static int thread_id(void){
		static std::atomic<int> id(0);
		thread_local int thread_id = std::atomic_fetch_add(&id, 1);
		return thread_id;
		// return hasher(std::this_thread::get_id());
	}
};

class Colog: public NoMove{
	std::mutex lk;
 public:
 	Colog(){
 		std::ios::sync_with_stdio(false);
		std::cin.tie(0);
 	}
 	template <typename T>
 	Colog& operator<<(const T& x){
 		std::lock_guard<std::mutex> local(lk);
 		std::cout << ThreadInfo::thread_string() << x << std::endl;
 		return *this;
 	}
 	template <class ...Args>
 	Colog& put_batch(Args... xs){
 		std::lock_guard<std::mutex> local(lk);
 		std::cout << ThreadInfo::thread_string();
 		int temp[] = { ((std::cout << xs), std::cout <<", " , 0 )... };
 		std::cout << std::endl;
 		return *this;
 	}
};

}

Colog colog;

namespace {
class Coreport: public NoMove{
 public:
 	Coreport() = default;
	bool Register(std::string key) {return true;}
	bool Add(std::string key, int count) {return true;}
};
} // np anonymous

Coreport coreport;

} // namespace coplus

