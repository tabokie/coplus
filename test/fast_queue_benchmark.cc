#include "coplus/thread_pool.h"
#include "coplus/fast_queue.h"
#include "coplus/colog.h"
#include "coplus/cotimer.h"

#include <functional>

using namespace coplus;

template <typename ElementType>
class NaiveQueue{
	// blocking
	std::mutex lock;
	std::queue<ElementType> data;
 public:
 	NaiveQueue() = default;
	~NaiveQueue() { }
	void push(ElementType item){
		std::lock_guard<std::mutex> local(lock);
		data.push(item);
	}
	void pop(ElementType& ret){
		std::lock_guard<std::mutex> local(lock);
		ret = data.front();
		data.pop();
	}
	size_t size(void){
		std::lock_guard<std::mutex> local(lock);
		return data.size();
	}
};

int NaiveQueueBenchmark(int threadSize, int opSize){
	NaiveQueue<int> queue;
	std::vector<std::thread> threads;
	cotimer.start();
	for(int k = 0; k < threadSize; k++){
		threads.push_back(std::move(std::thread([&]{
			for(int i = 0; i < opSize; i++){
				queue.push(9);
			}
		})));	
	}
	std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
	double time = cotimer.end();
	colog.put_batch("Naive version with ", threadSize, " threads and ", opSize, " ops: ", time);
	return 0;
}

int FastQueueBenchmark(int threadSize, int opSize){
	FastQueue<int> queue;
	std::vector<std::thread> threads;
	cotimer.start();
	for(int k = 0; k < threadSize; k++){
		threads.push_back(std::move(std::thread([&]{
			FastQueue<int>::ProducerHandle handle(&queue);
			for(int i = 0; i < opSize; i++){
				handle.push(9);
			}
		})));	
	}
	std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
	double time = cotimer.end();
	colog.put_batch("Fast version with ", threadSize, " threads and ", opSize, " ops: ", time);
	return 0;
}

int main(int argc, char** argv){
	int thread = std::thread::hardware_concurrency() - 1;
	int op = 10000;
	if(argc > 1){
		op = atoi(argv[1]);
	}
	NaiveQueueBenchmark(thread, op);
	FastQueueBenchmark(thread, op);
	return 0;
}