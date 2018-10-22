#include "coplus/coroutine.h"

namespace coplus{
void yield(){
	ToFiber(); // no input
	FiberData::public_status = Task::kReady; // reschedule to ready queue
}

void await(Trigger trigger){
	FiberData::trigger = trigger;
	FiberData::public_status = Task::kWaiting;
	ToFiber();
	FiberData::public_status = Task::kReady;
	return ;
}

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


