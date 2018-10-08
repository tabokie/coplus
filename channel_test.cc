#include <thread>
#include <iostream>
using std::cout;
using std::endl;
#include <memory>
#include <string>
#include <type_traits>
#include <gtest/gtest.h>

#include <condition_variable>

#include "threading.h"
#include "channel.h"
#include "colog.h"
#include "corand.h"

using namespace coplus;

// naive test for single thread
TEST(ChannelTest, BufferChannel){
	Channel<int, 3> chan;
	ThreadPool pool;	

	int a = corand.Int(100);
	int b = corand.Int(100);
	pool.submit(
		std::function<void(void)>([&]{
			colog << "Running";
			chan << a;
		})
	);

	pool.submit(
		std::function<void(void)>([&]{
			colog << "Running";
			chan << b; 
		})
	);


	auto recv1 = pool.submit(
		std::function<int(void)>([&]{
			colog << "Running";
			int ret;
			chan >> ret;
			return ret;
		})
	);
	auto recv2 = pool.submit(
		std::function<int(void)>([&]{
			colog << "Running";
			int ret;
			chan >> ret;
			return ret;
		})
	);

	colog << "waiting" ;
	EXPECT_EQ(a+b, recv1.get()+recv2.get());
	colog << "close" ;
	pool.close();
	chan.close();
}

TEST(ChannelTest, SyncChannel){
	SyncChannel<int> chan;
	int a = corand.Int(100);
	int b = corand.Int(100);
	auto recv1 = std::packaged_task<int()>(
		std::function<int(void)>([&]{
			colog << "Running recv1";
			int ret;
			chan >> ret;
			return ret;
		})
	);
	auto recv2 = std::packaged_task<int()>(
		std::function<int(void)>([&]{
			colog << "Running recv2";
			int ret;
			chan >> ret;
			return ret;
		})
	);
	auto sender1 = std::thread(
		std::function<void(void)>([&]{
			colog << "Running send1";
			chan << a;
		})
	);
	auto sender2 = std::thread(
		std::function<void(void)>([&]{
			colog << "Running send2";
			chan << b; 
		})
	);
	colog << "join 1";
	recv1();
	colog << "join 2";
	recv2();
	colog << "end 1";
	sender1.join();
	colog << "end 2";
	sender2.join();
	chan.close();

	EXPECT_EQ(recv1.get_future().get() + recv2.get_future().get(), a+b);
}

// int main(void){

// 	// Channel<int, 0> chan;
// 	SyncChannel<int> chan;
// 	cout << "Create two threads" << endl;
// 	auto recv1 = std::thread(
// 		std::function<void(void)>([&]{
// 			int ret;
// 			colog << " needs.";
// 			chan >> ret;
// 			colog << ("Thread 1 (recv): " + std::to_string(ret));
// 		})
// 	);
// 	auto recv2 = std::thread(
// 		std::function<void(void)>([&]{
// 			int ret;
// 			colog << " needs.";
// 			chan >> ret;
// 			colog << ("Thread 2 (recv): " + std::to_string(ret));
// 		})
// 	);
// 	auto sender1 = std::thread(
// 		std::function<void(void)>([&]{
// 			colog << " give.";
// 			chan << 1; 
// 			colog << "Thread 1 (sending)";
// 		})
// 	);
// 	auto sender2 = std::thread(
// 		std::function<void(void)>([&]{
// 			colog << " give.";
// 			chan << 2; 
// 			colog << "Thread 2 (sending)";
// 		})
// 	);
// 	sender1.join();
// 	recv1.join();
// 	sender2.join();
// 	recv2.join();
// 	chan.close();

// 	return 0;
// }