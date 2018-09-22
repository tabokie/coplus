#pragma once

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
	virtual ~TaskBase() { }
};

template <typename SchedulerType = std::thread>
class ThreadPool{
	std::vector<TaskBase> tasks;
	// std::vector<Machine> machines;
	SchedulerType scheduler;
 public:
 	ThreadPool(){ }
 	~ThreadPool(){ }
	template <typename T>
	static std::thread NewThread(std::function<T>&& func){
		std::thread temp(std::move(func));
		return std::move(temp);
	}

	// for function with return value
	template<
	typename FunctionType, 
	typename test = std::enable_if<std::bool_constant<!bool(std::is_void<std::result_of<FunctionType()>::type>::value)>::value>::type>
	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType&& f){
		using ResultType = typename std::result_of<FunctionType()>::type;
		std::packaged_task<ResultType()> packaged_f(std::move(f));
		auto ret = packaged_f.get_future();
		tasks.push_back(std::move(packaged_f));
		return ret;
	}
	// for void return function
	template<
	typename FunctionType, 
	typename test = std::enable_if<std::is_void<typename std::result_of<FunctionType()>::type>::value>::type >
	void submit(FunctionType&& f){
		tasks.push_back(std::move(f));
	}
};