#include "threading.h"
#include "fast_queue.h"
#include "colog.h"

#include <functional>

using namespace std;
using namespace coplus;

int TestPushOnlyFastStack(){
	PushOnlyFastStack<int> stack(200);
	int pushSize0 = corand.UInt(50);
	int pushSize1 = corand.UInt(50);
	int pushSize2 = corand.UInt(50);
	auto t0 = ThreadPool::NewThread(
		function<int(void)>([&]{
			for(int i = 0; i < pushSize0; i++)
				stack.push(9);
			return 0;
		})
	);
	auto t1 = ThreadPool::NewThread(
		function<int(void)>([&]{
			for(int i = 0; i < pushSize1; i++)
				stack.push(9);
			return 0;
		})
	);

	auto t2 = ThreadPool::NewThread(
		function<int(void)>([&]{
			for(int i = 0; i < pushSize2; i++)
				stack.push(9);
			return 0;
		})
	);

	t0.join();
	t1.join();
	t2.join();
	assert(stack.size() == pushSize0 + pushSize1 + pushSize2 || stack.size() == 200);
	// EXPECT_EQ(stack.size(), pushSize1 + pushSize0);
	return 0;
}

int TestFastStack(){
	FastStack<int> stack(100);
	int pushSize0 = corand.UInt(50);
	int pushSize1 = corand.UInt(50);
	int popSize0 = corand.UInt(pushSize0);
	int popSize1 = corand.UInt(pushSize1);
	auto t0 = ThreadPool::NewThread(
		function<int(void)>([&]{
			for(int i = 0; i < pushSize0; i++)
				while(!stack.push(9)){	}
			return 0;
		})
	);
	auto t1 = ThreadPool::NewThread(
		function<int(void)>([&]{
			for(int i = 0; i < pushSize1; i++)
				while(!stack.push(9)){	}
			return 0;
		})
	);
	auto t2 = ThreadPool::NewThread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize0; i++)
				while(!stack.pop(ret)){ };
			return 0;
		})
	);
	auto t3 = ThreadPool::NewThread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize1; i++)
				while(!stack.pop(ret)){ };
			return 0;
		})
	);

	t0.join();
	t1.join();
	t2.join();
	t3.join();
	assert(stack.size() == pushSize0 + pushSize1 - popSize1 - popSize0 );
}

int main(void){
	// return TestPushOnlyFastStack();
	auto ret = TestFastStack();
	cout << "Safely exit..." << endl;
	return ret;
}