#include <thread>
#include <iostream>
using std::cout;
using std::endl;
#include <memory>
#include <string>
#include <type_traits>
#include <condition_variable>

#include "thread_pool.hpp"


int main(void){


	// Channel<int, 0> chan;
	SyncChannel<int> chan;
	cout << "Create two threads" << endl;
	auto recv1 = ThreadPool::NewThread(
		std::function<void(void)>([&]{
			int ret;
			cout << std::this_thread::get_id() << " needs." << std::endl;
			chan >> ret;
			cout << "Thread 1 (recv): " << ret << endl;
		})
	);
	auto recv2 = ThreadPool::NewThread(
		std::function<void(void)>([&]{
			int ret;
			cout << std::this_thread::get_id() << " needs." << std::endl;
			chan >> ret;
			cout << "Thread 2 (recv): " << ret << endl;
		})
	);
	auto sender1 = ThreadPool::NewThread(
		std::function<void(void)>([&]{
			cout << std::this_thread::get_id() << " give." << std::endl;
			chan << 1; 
			cout << "Thread 1 (sending)" << endl;
		})
	);
	auto sender2 = ThreadPool::NewThread(
		std::function<void(void)>([&]{
			cout << std::this_thread::get_id() << " give." << std::endl;
			chan << 2; 
			cout << "Thread 2 (sending)" << endl;
		})
	);
	sender1.join();
	recv1.join();
	sender2.join();
	recv2.join();
	chan.close();

	return 0;
}
