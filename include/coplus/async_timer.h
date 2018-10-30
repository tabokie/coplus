#pragma once

#include "coplus/port.h"
#include "coplus/fiber_task.h"

#ifdef WIN_PORT
#include <Windows.h>
#endif

namespace coplus{

class AsyncTimer{
 public:
	static void await(float seconds, Trigger trigger);
	static void hr_await(float seconds, Trigger trigger);
};

} // namespace coplus
