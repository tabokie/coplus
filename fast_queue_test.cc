#include "threading.h"
#include "fast_queue.h"
#include "colog.h"

#include <functional>

using namespace std;
using namespace coplus;

template <typename ElementType>
struct TestPushOnlyFastStack{
	int operator()(){
		PushOnlyFastStack<ElementType> stack(200);
		int pushSize0 = corand.UInt(50);
		int pushSize1 = corand.UInt(50);
		int pushSize2 = corand.UInt(50);
		int pushSize = pushSize0 + pushSize1 + pushSize2;
		auto t0 = std::thread(
			function<int(void)>([&]{
				for(int i = 0; i < pushSize0; i++)
					stack.push(ElementType());
				return 0;
			})
		);
		auto t1 = std::thread(
			function<int(void)>([&]{
				for(int i = 0; i < pushSize1; i++)
					stack.push(ElementType());
				return 0;
			})
		);

		auto t2 = std::thread(
			function<int(void)>([&]{
				for(int i = 0; i < pushSize2; i++)
					stack.push(ElementType());
				return 0;
			})
		);

		auto t3 = std::thread(
			function<int(void)>([&]{
				ElementType ret;
				for(int i = 0; i < pushSize; i++){
					bool status = stack.get(i, ret);
					if(!status){
						colog << "Ret false";
					}
				}
				return 0;
			})
		);

		t0.join();
		t1.join();
		t2.join();
		t3.join();
		assert(stack.size() == pushSize0 + pushSize1 + pushSize2 || stack.size() == 200);
		// EXPECT_EQ(stack.size(), pushSize1 + pushSize0);
		return 0;
	}
};

template <typename ElementType>
struct TestPushOnlyFastStack<std::unique_ptr<ElementType>>{
	int operator()(){
		PushOnlyFastStack<std::unique_ptr<ElementType>> stack(200);
		int pushSize0 = corand.UInt(50);
		int pushSize1 = corand.UInt(50);
		int pushSize2 = corand.UInt(50);
		int pushSize = pushSize0 + pushSize1 + pushSize2;
		auto t0 = std::thread(
			function<int(void)>([&]{
				for(int i = 0; i < pushSize0; i++)
					stack.push(new ElementType());
				return 0;
			})
		);
		auto t1 = std::thread(
			function<int(void)>([&]{
				for(int i = 0; i < pushSize1; i++)
					stack.push(new ElementType());
				return 0;
			})
		);

		auto t2 = std::thread(
			function<int(void)>([&]{
				for(int i = 0; i < pushSize2; i++)
					stack.push(new ElementType());
				return 0;
			})
		);

		auto t3 = std::thread(
			function<int(void)>([&]{
				ElementType* ret;
				for(int i = 0; i < pushSize; i++){
					bool status = stack.get(i, ret);
					if(!status){
						colog << "Ret false";
					}
					else if(!ret){
						colog << "Get a nullptr";
					}
				}
				return 0;
			})
		);

		t0.join();
		t1.join();
		t2.join();
		t3.join();
		assert(stack.size() == pushSize0 + pushSize1 + pushSize2 || stack.size() == 200);
		// EXPECT_EQ(stack.size(), pushSize1 + pushSize0);
		return 0;
	}
};

int TestFastStack(){
	FastStack<int> stack(100);
	int pushSize0 = corand.UInt(50);
	int pushSize1 = corand.UInt(50);
	int popSize0 = corand.UInt(pushSize0);
	int popSize1 = corand.UInt(pushSize1);
	auto t0 = std::thread(
		function<int(void)>([&]{
			for(int i = 0; i < pushSize0; i++)
				while(!stack.push(9)){	}
			return 0;
		})
	);
	auto t1 = std::thread(
		function<int(void)>([&]{
			for(int i = 0; i < pushSize1; i++)
				while(!stack.push(9)){	}
			return 0;
		})
	);
	auto t2 = std::thread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize0; i++)
				while(!stack.pop(ret)){ };
			return 0;
		})
	);
	auto t3 = std::thread(
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
	return 0;
}

int TestLocalQueue(){
	FastLocalQueue<int, 50> queue;
	int pushSize = corand.UInt(50);
	int popSize0 = corand.UInt(pushSize / 3);
	int popSize1 = corand.UInt(pushSize - popSize0);
	// int popSize2 = corand.UInt(pushSize - popSize0 - popSize1);
	int popSize2 = (pushSize - popSize0 - popSize1);
	auto t0 = std::thread(
		function<int(void)>([&]{
			for(int i = 0; i < pushSize; i++)
				while(!queue.push(9)){}
			return 0;
		})
	);
	auto t1 = std::thread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize0; i++)
				while(!queue.pop(ret)){}
			return 0;
		})
	);
	auto t2 = std::thread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize1; i++)
				while(!queue.pop(ret)){};
			return 0;
		})
	);
	auto t3 = std::thread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize2; i++)
				while(!queue.pop(ret)){};
			return 0;
		})
	);

	t0.join();
	t1.join();
	t2.join();
	t3.join();
	assert(queue.size() == pushSize - popSize2 - popSize1 - popSize0 );
	return 0;
}

int TestFastQueue(void){
	FastQueue<int> queue;
	int pushSize0 = corand.UInt(50);
	int pushSize1 = corand.UInt(50);
	int pushSize = pushSize0 + pushSize1;
	int popSize0 = corand.UInt(pushSize / 3);
	int popSize1 = corand.UInt(pushSize - popSize0);
	int popSize2 = (pushSize - popSize0 - popSize1);
	// hard pusher
	auto t0 = std::thread(
		function<int(void)>([&]{
			FastQueue<int>::ProducerHandle handle(&queue);
			for(int i = 0; i < pushSize0; i++){
				if(!handle.push(9)){
					if(!handle.push_hard(9)){
						colog << "Critical: push hard failed";
					}
				}
			}
			return 0;
		})
	);
	// weak pusher
	auto t1 = std::thread(
		function<int(void)>([&]{
			FastQueue<int>::ProducerHandle handle(&queue);
			for(int i = 0; i < pushSize1; i++){
				if(!handle.push(9)){
					if(!handle.push_hard(9)){
						colog << "Critical: push hard failed";
					}
				}
			}
			return 0;
		})
	);
	// hard popper
	auto t2 = std::thread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize1; i++)
				while(!queue.pop_hard(ret)){ };
			return 0;
		})
	);
	auto t3 = std::thread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize2; i++)
				while(!queue.pop_hard(ret)){ };
			return 0;
		})
	);
	auto t4 = std::thread(
		function<int(void)>([&]{
			int ret;
			for(int i = 0; i < popSize0; i++)
				while(!queue.pop_hard(ret)){ };
			return 0;
		})
	);

	t0.join();
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	assert(queue.element_count() == 0 );
	return 0;
}

int main(void){
	// auto ret = TestPushOnlyFastStack<std::unique_ptr<int>, int*>{}();
	// auto ret = TestPushOnlyFastStack<int>{}();
	auto ret = TestFastQueue();
	cout << "Safely exit..." << endl;
	return 0;
}