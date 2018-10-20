#pragma once

#include <type_traits>

#include "fiber_port.h"

namespace coplus{

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

void __stdcall _co_proxy(void* pData){
	Task* p = (Task*)pData;
	p->raw_call(); // /*yield();*/ return;
	ToFiber();
	// return; // if return here, fiber is ended / undefined
}

template <typename FunctionType, typename test = typename std::result_of<FunctionType()>::type>
struct FiberTask: public Task{
	RawFiber ret_routine = NilFiber;
	RawFiber work_routine = NilFiber;
	bool finished = false;
	int cur;
	FiberTask(FunctionType&& f): f_(std::move(f)) { static int id = 0; cur = id++; }
	~FiberTask() { }
	bool call(void){
		ret_routine = CurrentFiber();
		if(work_routine == NilFiber)work_routine = NewFiber(_co_proxy, this);
		ToFiber(work_routine);
		/* here _co_proxy(this) */
		// if(CurrentFiber() == ret_routine){
		// 	colog << "return";
		// 	finished = false;
		// }
		// else{
		// 	colog << "finish";
		// 	finished = true;
		// 	ToFiber(ret_routine);
		// }
		return finished;
	}
	void raw_call(){f_();finished = true;}
	FunctionType f_;
};

}