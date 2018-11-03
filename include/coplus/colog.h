#pragma once

#include <thread>
#include <cstdio>
#include <mutex>
#include <iostream>
#include <functional>
#include <string>
#include <atomic>
#include <cstdio>

#include "coplus/util.h"
#include "coplus/port.h"
#ifdef WIN_PORT
#include <Windows.h>
#endif

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

#ifdef WIN_PORT
struct ConsoleColor {
	enum ConsoleForegroundColor	{
		enmCFC_Red          = FOREGROUND_INTENSITY | FOREGROUND_RED,
		enmCFC_Green        = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
		enmCFC_Blue         = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
		enmCFC_Yellow       = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
		enmCFC_Purple       = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
		enmCFC_Cyan         = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
		enmCFC_Gray         = FOREGROUND_INTENSITY,
		enmCFC_White        = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
		enmCFC_HighWhite    = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
		enmCFC_Black        = 0,
	};
	enum ConsoleBackGroundColor	{
		enmCBC_Red          = BACKGROUND_INTENSITY | BACKGROUND_RED,
		enmCBC_Green        = BACKGROUND_INTENSITY | BACKGROUND_GREEN,
		enmCBC_Blue         = BACKGROUND_INTENSITY | BACKGROUND_BLUE,
		enmCBC_Yellow       = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,
		enmCBC_Purple       = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE,
		enmCBC_Cyan         = BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
		enmCBC_White        = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
		enmCBC_HighWhite    = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
		enmCBC_Black        = 0,
	};
	void operator()(int code){
		static HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		switch(code){
			case 0:SetConsoleTextAttribute(handle, enmCFC_White | enmCBC_Black);break;
			case 1:SetConsoleTextAttribute(handle, enmCFC_Yellow | enmCBC_Black);break;
			case 2:SetConsoleTextAttribute(handle, enmCFC_Black | enmCBC_Yellow);break;
			case 3:SetConsoleTextAttribute(handle, enmCFC_Black | enmCBC_Red);break;
		}
	}
};
#endif

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
#ifdef WIN_PORT
	template <class ...Args>
	Colog& highlight(Args... xs){
		std::lock_guard<std::mutex> local(lk);
		ConsoleColor{}(1);
		std::cout << ThreadInfo::thread_string();
		int temp[] = { ((std::cout << xs), std::cout <<", " , 0 )... };
		std::cout << std::endl;
		ConsoleColor{}(0);
		return *this;
	}
	template <class ...Args>
	Colog& warning(Args... xs){
		std::lock_guard<std::mutex> local(lk);
		ConsoleColor{}(2);
		std::cout << ThreadInfo::thread_string();
		int temp[] = { ((std::cout << xs), std::cout <<", " , 0 )... };
		std::cout << std::endl;
		ConsoleColor{}(0);
		return *this;
	}
	template <class ...Args>
	Colog& error(Args... xs){
		std::lock_guard<std::mutex> local(lk);
		ConsoleColor{}(3);
		std::cout << ThreadInfo::thread_string();
		int temp[] = { ((std::cout << xs), std::cout <<", " , 0 )... };
		std::cout << std::endl;
		ConsoleColor{}(0);
		return *this;
	}
#endif
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

