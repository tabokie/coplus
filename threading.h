#ifndef COPLUS_THREADING_H_
#define COPLUS_THREADING_H_

#include <vector>
#include <thread>
#include <queue>
#include <type_traits>
#include <future>
#include <algorithm>
#include <typeinfo>

#include "fiber_port.h"
#include "util.h"
#include "colog.h"
#include "fast_queue.h"

namespace coplus {

struct Task: public NoMove {
	virtual bool call() = 0; // return false if not completed
	virtual ~Task() {}
	virtual void raw_call() = 0;
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
	void raw_call(){f_();}
	FunctionType f_;
};

// fiber
void __stdcall _co_proxy(void* pData);
void _co_yield(RawFiber f);
#define go(_thread_pool)									do{void* _p_go_base = (void*)(new RawFiber());_thread_pool.submit(_p_go_base, _rm_lbracket_ad_rclosure
#define tgo(_thread_pool, _tracer)				do{void* _p_go_base = (void*)(new RawFiber());_tracer = _thread_pool.submit(_p_go_base, _rm_lbracket_ad_rclosure
#define _rm_lbracket_ad_rclosure(x)				x);}while(0)
#define yield															_co_yield(*((RawFiber*)_p_go_base))

template <typename FunctionType, typename test = typename std::result_of<FunctionType()>::type>
struct FiberTask: public Task{
	std::unique_ptr<RawFiber> ret_routine = nullptr;
	RawFiber work_routine = NilFiber;
	bool finished;
	int cur;
	FiberTask(void* p, FunctionType&& f):ret_routine((RawFiber*)p), f_(std::move(f)) { static int id = 0; cur = id++; }
	~FiberTask() { }
	bool call(void){
		*ret_routine = CurrentFiber();
		if(work_routine == NilFiber)work_routine = NewFiber(_co_proxy, this);
		ToFiber(work_routine);
		if(CurrentFiber() == *ret_routine){
			finished = false;
		}
		else{
			finished = true;
			ToFiber(*ret_routine);
		}
		return finished;
	}
	void raw_call(){f_();}
	FunctionType f_;
};

void __stdcall _co_proxy(void* pData){
	Task* p = (Task*)pData;
	p->raw_call();
}
void _co_yield(RawFiber f){
	ToFiber(f);
}

class Machine: public NoCopy{ // simple encap of thread
	std::thread t_;
	std::atomic<bool>* close_; // using pointer for MoveAssign used in container
	int* job_count_;
	// asserting access to close happen in valid range
	// asserting thread.empty == close_
 public:
	template <typename FunctionType>
	Machine(FunctionType&& scheduler): close_(new std::atomic<bool>(false)), job_count_(new int(0)){
		t_ = std::move(std::thread([&, close=close_, schedule=std::move(scheduler), count=job_count_]() mutable{
			InitEnv();
	 		while( !close->load() ){
				if (schedule()){
					(*count) ++;
				}
	 		}
		}));
	}
	// required for container
	Machine(Machine&& rhs) : t_(std::move(rhs.t_)) {
		close_ = rhs.close_;
		rhs.close_ = nullptr;
		job_count_ = rhs.job_count_;
		rhs.job_count_ = nullptr;
	}
	// for container resize larger which is deprecated
 	Machine(): close_(new std::atomic<bool>(true)) { }
 	// just a matching interface
 	template <typename FunctionType>
 	bool restart(FunctionType&& scheduler){
 		if(!close_->load())return false;
 		t_ = std::move(std::thread([&, close=close_, schedule=std::move(scheduler), count=job_count_]() mutable{
			InitEnv();
	 		while( !close->load() ){
				if (schedule()){
					(*count) ++;
				}
	 		}
		}));
		return true;
 	}
	~Machine() { delete close_; }
	void join(void){
		if(close_->load())return; // already closed
		atomic_store(close_, true); // send out signal to thread
		t_.join();
		return ;
	}
	void report(void){
		colog << (std::to_string(*job_count_) + " jobs scheduled");
		return ;
	}
};

/*
// landmark for coroutine //
// Coroutine have to be stored globally into a buffer
Fiber::yield(condv){
	condv->register(GetCurrentFiber());
	SwitchToFiber(mainFiber); // mainFiber is parameter passed to function
}
Fiber::return(){
	GetFiberData()->SetEmpty();
	// must explicit switch back to fiber
	SwitchToFiber(caller);
}
condv::notify(){
	SwitchToFiber(registered[0]); // can call fiber created by other thread
}
Machine::Schedule(){
	if(task.coroutine());
	moveTaskToStatic(task); // maybe init in static area, saving archive shared pointer
	task->addData(thisFiber);
	sonFiber = CreateFiber(0, &(task.front()->call(LPVOID)), task->dataPtr());
	SwitchToFiber(sonFiber);
	pop(); // already moved to static area
	// can be called again by id?
}
Thread::Cleaner(){
	for(ptr : GlobalArea){
		if(ptr->dead())erase(ptr);
	}
}
*/

class ThreadPool: public NoMove{

	FastQueue<std::shared_ptr<Task>> tasks;
	std::vector<Machine> machines;
	size_t threadSize;
	std::mutex mutating; // for structure mutating, lock all
 public:
 	ThreadPool(){
 		threadSize = std::thread::hardware_concurrency(); // one for main thread
 		for(int i = 0; i < threadSize - 1; i++){
 			machines.push_back( //std::thread(
 				[&]() -> bool {
 					std::shared_ptr<Task> current;
 					bool ret = tasks.pop_hard(current);
 					if(ret){
 						if(!current->call()){
 							// back to pool
							FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
							if(!handle.push_hard(current)){
								colog << "push fail";
							}
 						}
 						return true;
 					}
 					return false;
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
	 				[&]()-> bool{
	 					std::shared_ptr<Task> current;
	 					bool ret = tasks.pop_hard(current);
	 					if(ret){
	 						current->call();
	 						return true;
	 					}
	 					return false;
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
 	void report(void){
 		colog << std::to_string(tasks.element_count()) + " jobs left";
		std::for_each(machines.begin(), machines.end(), std::mem_fn(&Machine::report));
 	}
	// for function with return value
	template<
	typename FunctionType, 
	typename test = typename std::enable_if<!bool(std::is_void<typename std::result_of<FunctionType()>::type>::value)>::type>
	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType&& f){
		// @BUG: thread_local outlive the FastQueue object, use temporary handle instead
		FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
		using ResultType = typename std::result_of<FunctionType()>::type;
		std::packaged_task<ResultType()> packaged_f(std::move(f));
		auto ret = (packaged_f.get_future());
		bool status = handle.push_hard(std::make_shared<FunctionTask<std::packaged_task<ResultType()>>>(std::move(packaged_f)));
		if(!status){
			colog << "Push failed";
		}
		return (ret);
	}
	// for void return function
	template<
	typename FunctionType, 
	typename test = std::enable_if<std::is_void<typename std::result_of<FunctionType()>::type>::value>::type >
	void submit(FunctionType&& f){
		FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
		bool status = handle.push_hard(std::make_shared<FunctionTask<FunctionType>>(std::move(f)));
		if(!status){
			colog << "Push failed";
		}
	}
	// for void fiber
	template<
	typename FunctionType, 
	typename test = typename std::enable_if<std::is_void<typename std::result_of<FunctionType()>::type>::value>::type >
	void submit(void* p, FunctionType&& f){
		FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
		bool status = handle.push_hard(std::make_shared<FiberTask<FunctionType>>(p, std::move(f)));
		if(!status){
			colog << "Push failed";
		}
	}
	// for return fiber
	template<
	typename FunctionType, 
	typename test = typename std::enable_if<!bool(std::is_void<typename std::result_of<FunctionType()>::type>::value)>::type >
	std::future<typename std::result_of<FunctionType()>::type> submit(void* p, FunctionType&& f){
		FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
		using ResultType = typename std::result_of<FunctionType()>::type;
		std::packaged_task<ResultType()> packaged_f(std::move(f));
		auto ret = (packaged_f.get_future());
		bool status = handle.push_hard(std::make_shared<FiberTask<std::packaged_task<ResultType()>>>(p, std::move(packaged_f)));
		if(!status){
			colog << "Push failed";
		}
		return (ret);
	}


};

} // namespace coplus


#endif // COPLUS_THREADING_H_