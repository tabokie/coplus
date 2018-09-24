#ifndef COPLUS_THREADING_H_
#define COPLUS_THREADING_H_

#include <vector>
#include <thread>
#include <queue>
#include <type_traits>
#include <future>

#include "colog.h"

namespace coplus {

struct TaskBase {
	struct TaskFunctionBase {
		virtual void call() = 0;
		virtual ~TaskFunctionBase() {}
	};
	std::unique_ptr<TaskFunctionBase> function;
	template <typename FunctionType>
	struct TaskFunction: public TaskFunctionBase	{
		TaskFunction(FunctionType&& f): f_(std::move(f)) { }
		~TaskFunction(){ }
		void call(){
			f_();
		}        
		FunctionType f_;
	};
	template <typename FunctionType>
	TaskBase(FunctionType&& f): function(new TaskFunction<FunctionType>(std::move(f))) { }
	TaskBase(TaskBase&& that): function(std::move(that.function)) { }
	TaskBase(const TaskBase&) = delete;
	void operator()(){
		function->call();
	}
	virtual ~TaskBase() { }
};

template <typename SchedulerType = std::thread>
class ThreadPool{
	std::queue<TaskBase> tasks;
	// std::vector<Machine> machines;
	SchedulerType scheduler;
	std::atomic<bool> closed;
 public:
 	ThreadPool(): scheduler([&](){
 		colog << "scheduler on";
 		colog << tasks.size();
 		std::atomic_store_explicit(&closed, false, std::memory_order_relaxed);
 		while(!std::atomic_load(&closed) || tasks.size() ){
 			colog << "loop";	
 			if(tasks.size()){
	 			tasks.front()();
	 			tasks.pop();	
 			}
 		}
 		colog << "scheduler off" ;
 	}){
 		scheduler.detach();
 	}
 	~ThreadPool(){ }
 	void close(void){
 		std::atomic_store(&closed, true);
 	}
 	// quick new
	template <typename T>
	static std::thread NewThread(std::function<T>&& func){
		std::thread temp(std::move(func));
		return std::move(temp);
	}
	template <typename FunctionType>
	static std::packaged_task<typename std::result_of<FunctionType()>::type()> NewFuture(FunctionType&& f){
		using ResultType = typename std::result_of<FunctionType()>::type;
		return std::packaged_task<ResultType()>(std::move(f));
	}

	// for function with return value
	template<
	typename FunctionType, 
	typename test = typename std::enable_if<!bool(std::is_void<typename std::result_of<FunctionType()>::type>::value)>::type>
	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType&& f){
		using ResultType = typename std::result_of<FunctionType()>::type;
		std::packaged_task<ResultType()> packaged_f(std::move(f));
		auto ret = packaged_f.get_future();
		tasks.push(std::move(packaged_f));
		return ret;
	}
	// for void return function
	template<
	typename FunctionType, 
	typename test = typename std::enable_if<std::is_void<typename std::result_of<FunctionType()>::type>::value>::type >
	void submit(FunctionType&& f){
		tasks.push(std::move(f));
	}

};


} // namespace coplus


#endif // COPLUS_THREADING_H_