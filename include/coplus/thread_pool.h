#ifndef COPLUS_THREADING_H_
#define COPLUS_THREADING_H_

#include <vector>
#include <thread>
#include <queue>
#include <type_traits>
#include <future>
#include <algorithm>
#include <typeinfo>

#include "coplus/util.h"
#include "coplus/colog.h"
#include "coplus/fast_queue.h"
#include "coplus/task.h"
#include "coplus/fiber_task.h"
#include "coplus/machine.h"

namespace coplus {

class ThreadPool: public NoMove{
	// Table<std::shared_ptr<Task>> global_tasks;
	std::vector<Machine> machines;
	size_t threadSize;
	std::mutex mutating; // for structure mutating, lock all
 public:
	FastQueue<std::shared_ptr<Task>> tasks;
 	std::mutex protect_waiters;
	std::vector<std::shared_ptr<Task>> wait_queue;
 	ThreadPool(){
 		threadSize = std::thread::hardware_concurrency(); // one for main thread
 		for(int i = 0; i < threadSize - 1; i++){
 			machines.push_back( //std::thread(
 				[&]() -> bool {
 					std::shared_ptr<Task> current;
 					bool ret = tasks.pop_hard(current);
 					if(ret){
 						switch(current->call()){
 							case Task::kFinished:
 							break;
 							case Task::kReady:
 							{
	 							// back to pool
	 							FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
								if(!handle.push_hard(current)){
									colog << "push fail";
								}	
 							}
 							break;
 							case Task::kWaiting:{
	 							protect_waiters.lock();
	 							int id = wait_queue.size();
	 							wait_queue.push_back(current);
	 							protect_waiters.unlock();
	 							FiberData::trigger->Register(id);
	 							FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
	 							if(!handle.push_hard(FiberData::wait_for_task)){
	 								colog << "push fail";
	 							}	
 							}
 							break;
 						}
 						return true;
 					}
 					// fail to execute
 					return false;
 				}
 			);
 		}
 	}
 	~ThreadPool(){
 		close();
 		report();
 	}
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
	void go(FunctionType&& f){
		FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
		bool status = handle.push_hard(std::make_shared<FiberTask<FunctionType>>(std::move(f)));
		if(!status){
			colog << "Push failed";
		}
	}
	// for fiber with return value
	template<
	typename FunctionType, 
	typename test = typename std::enable_if<!bool(std::is_void<typename std::result_of<FunctionType()>::type>::value)>::type >
	std::future<typename std::result_of<FunctionType()>::type> go(FunctionType&& f){
		FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&tasks);
		using ResultType = typename std::result_of<FunctionType()>::type;
		std::packaged_task<ResultType()> packaged_f(std::move(f));
		auto ret = (packaged_f.get_future());
		bool status = handle.push_hard(std::make_shared<FiberTask<std::packaged_task<ResultType()>>>(std::move(packaged_f)));
		if(!status){
			colog << "Push failed";
		}
		return (ret);
	}


}; // ThreadPool

ThreadPool kPool;


} // namespace coplus


#endif // COPLUS_THREADING_H_