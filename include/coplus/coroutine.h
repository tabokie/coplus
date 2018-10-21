#pragma once

#include "coplus/thread_pool.h"

namespace coplus{

#define go(x)				kPool.go(x);
#define submit(x)		kPool.go(x);

// yield //
// semantic: hold for visible interval and run
void yield(){
	ToFiber(); // no input
	FiberData::public_status = Task::kReady; // reschedule to ready queue
}
// await //
// semantic: run after target in global order, target is endable

template <
typename FunctionType, 
typename test = typename std::enable_if<std::is_void<typename std::result_of<FunctionType()>::type>::value>::type>
void await(FunctionType&& func){
	Trigger temp = NewTrigger();
	FiberData::trigger = temp;
	FiberData::wait_for_task = std::make_shared<FiberTask<std::function<void(void)>>>([=, f=std::move(func)](){
		func();
		notify(temp);
		return ;
	});
	// FiberData::wait_for_task = std::make_shared<FiberTask<FunctionType>>(std::move(func));
	FiberData::public_status = Task::kWaiting;
	ToFiber();
	FiberData::public_status = Task::kReady;
}

template<
typename FunctionType, 
typename test = typename std::enable_if<!bool(std::is_void<typename std::result_of<FunctionType()>::type>::value)>::type >
std::future<typename std::result_of<FunctionType()>::type> await(FunctionType&& func){
	Trigger temp = NewTrigger();
	FiberData::trigger = temp;
	using ResultType = typename std::result_of<FunctionType()>::type;
	std::packaged_task<ResultType()> packaged_f([=, f=std::move(func)]{
		auto ret = f();
		notify(temp);
		return ret;
	});
	auto ret = (packaged_f.get_future());
	FiberData::wait_for_task = std::make_shared<FiberTask<std::packaged_task<ResultType()>>>(std::move(packaged_f));
	FiberData::public_status = Task::kWaiting;
	ToFiber();
	FiberData::public_status = Task::kReady;
	return ret;
}

void await(Trigger trigger){
	FiberData::trigger = trigger;
	FiberData::public_status = Task::kWaiting;
	ToFiber();
	FiberData::public_status = Task::kReady;
	return ;
}


// trigger //
// semantic: give target the permission to run, no ordering in current context	
void notify(Trigger trigger){
	if(!trigger)return ;
	std::lock_guard<std::mutex> local(trigger->lk);
	// std::lock_guard<std::mutex> protect_waiters_local(kPool.protect_waiters);
	FastQueue<std::shared_ptr<Task>>::ProducerHandle handle(&kPool.tasks);
	std::shared_ptr<Task> pTask;
	for(auto id: trigger->waiters){
		bool status = kPool.wait_queue.get(id, pTask);
		if(!status){
			colog << "wait queue fail";
			break;
		}
		status = handle.push_hard(pTask);
		// bool status = handle.push_hard(kPool.wait_queue[id]);
		if(!status){
			colog << "push fail";
		}
	}
	return ;
}



}



