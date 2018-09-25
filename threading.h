#ifndef COPLUS_THREADING_H_
#define COPLUS_THREADING_H_

#include <vector>
#include <thread>
#include <queue>
#include <type_traits>
#include <future>
#include <algorithm>

#include "colog.h"

namespace coplus {

struct Task {
	virtual bool call() = 0; // return false if not completed
	virtual ~Task() {}
};

// blocking function
template <typename FunctionType>
struct FunctionTask: public Task	{
	FunctionTask(FunctionType&& f): f_(std::move(f)) { }
	~FunctionTask(){ }
	bool call(){
		f_();
		return true;
	}
	FunctionType f_;
};

// fiber
struct FiberTask: public Task{
	FiberTask() = default;
	~FiberTask() { }
	bool call(){
		// not implemented
		return false;
	}
};

class Machine{ // simple encap of thread
	std::thread t_;
	std::atomic<bool>* close_; // using pointer for CopyAssign used in container
	// asserting access to close happen in valid range
	// asserting thread.empty == close_
 public:
	template <typename FunctionType>
	Machine(FunctionType&& scheduler): close_(new std::atomic<bool>(false)){
		t_ = std::move(std::thread([&, close=close_, schedule=std::move(scheduler)](){
	 		while( !close->load() ){
				schedule();
	 		}
		}));
	}
	// required for container
	Machine(Machine&& rhs) : t_(std::move(rhs.t_)){
		close_ = rhs.close_;
		rhs.close_ = nullptr;
	}
	Machine(const Machine& rhs) = delete;
	// for container resize larger which is deprecated
 	Machine(): close_(new std::atomic<bool>(true)) { }
 	// just a matching interface
 	template <typename FunctionType>
 	bool restart(FunctionType&& scheduler){
 		if(!close_->load())return false;
 		t_ = std::move(std::thread([&, close=close_, schedule=std::move(scheduler)](){
	 		while( !close->load() ){
				schedule();
	 		}
		}));
		return true;
 	}
	~Machine() { delete close_; }
	void join(void){
		if(close_->load())return; // already closed
		atomic_store(close_, true); // send out signal to thread
		t_.join();
	}
};

class ThreadPool{
	std::queue<std::shared_ptr<Task> > tasks;
	std::vector<Machine> machines;
	// std::vector<std::thread> machines;
	std::mutex lock;
	size_t threadSize;
	std::mutex mutating;
 public:
 	ThreadPool(){
 		threadSize = std::thread::hardware_concurrency();
 		for(int i = 0; i < threadSize; i++){
 			machines.push_back( //std::thread(
 				[&](){
 					lock.lock();
		 			if(tasks.size()){
			 			tasks.front()->call();
			 			tasks.pop();
		 			}
		 			lock.unlock();
 				}
 			);
 		}
 	}
 	~ThreadPool(){ }
 	void resize(size_t newSize){
 		std::lock_guard<std::mutex> local(mutating);
 		if(newSize < threadSize){
			std::for_each(machines.begin() + newSize, machines.end(), std::mem_fn(&Machine::join));
 			// machines.erase(machines.begin() + newSize, machines.end()); // cannot use erase
 			machines.resize(newSize);
 		}
 		else if(newSize > threadSize){
 			for(int i = threadSize; i < newSize; i++){
 				machines.push_back(
	 				[&](){
	 					lock.lock();
			 			if(tasks.size()){
				 			tasks.front()->call();
				 			tasks.pop();
			 			}
			 			lock.unlock();
	 				}
	 			);
 			}
 		}
 		threadSize = newSize;
 		return ;
 	}
 	void close(void){
		std::for_each(machines.begin(), machines.end(), std::mem_fn(&Machine::join));
 	}
 	// quick new
	template <typename T>
	static std::thread NewThread(std::function<T>&& func){
		std::thread temp(std::move(func));
		return std::move(temp);
	}

	// for function with return value
	template<
	typename FunctionType, 
	typename test = typename std::enable_if<!bool(std::is_void<typename std::result_of<FunctionType()>::type>::value)>::type>
	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType&& f){
		using ResultType = typename std::result_of<FunctionType()>::type;
		std::packaged_task<ResultType()> packaged_f(std::move(f));
		auto ret = packaged_f.get_future();
		std::lock_guard<std::mutex> local(lock);
		tasks.push(std::make_shared<FunctionTask<std::packaged_task<ResultType()>>>(std::move(packaged_f)));
		return ret;
	}
	// for void return function
	template<
	typename FunctionType, 
	typename test = typename std::enable_if<std::is_void<typename std::result_of<FunctionType()>::type>::value>::type >
	void submit(FunctionType&& f){
		std::lock_guard<std::mutex> local(lock);
		tasks.push(std::make_shared<FunctionTask<FunctionType>>(std::move(f)));
	}

};


} // namespace coplus


#endif // COPLUS_THREADING_H_