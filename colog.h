#pragma once

#include <thread>
#include <cstdio>
#include <mutex>
#include <iostream>
#include <functional>

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
		thread_local int thread_id = std::atomic_fetch_add_explicit(&id, 1, std::memory_order_relaxed);
		return thread_id;
		// return hasher(std::this_thread::get_id());
	}
};

class Colog{
	std::mutex lk;
 public:
 	template <typename T>
 	Colog& operator<<(const T& x){
 		std::lock_guard<std::mutex> local(lk);
 		std::cout << ThreadInfo::thread_string() << x << std::endl;
 		return *this;
 	}
};

}

// struct Colog {
// 	FatalLog Fatal;
// 	InfoLog Info;
// 	int LogLevel;
// 	Colog(int level = 0): LogLevel(level){ }
// 	~Colog() { }
// };

Colog colog;


}

