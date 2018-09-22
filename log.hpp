#pragma once

#include <thread>
#include <cstdio>
#include <mutex>
#include <iostream>
#include <functional>

std::hash<std::thread::id> hasher;

// colored logging
class SyncLog{
	std::mutex lk;
 public:
	const SyncLog& operator<<(const char* data){
		char temp[100]; // for now
		sprintf(temp, "[thread %d]%s\n", hasher(std::this_thread::get_id()), data);
		std::lock_guard<std::mutex> guard(lk);
		std::cout << temp << std::endl;
		return *this;
	}

};

SyncLog slog; // single instance