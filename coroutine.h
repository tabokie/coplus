#pragma once

#include "colog.h"
#include "fiber_port.h"
#include "threading.h"

namespace coplus{

#define go(x)				kPool.go(x);
#define submit(x)		kPool.go(x);

// yield //
// semantic: hold for visible interval and run
void yield(){
	ToFiber(); // no input
}
// await //
// semantic: run after target in global order, target is endable
// implementation:
// add target as a task, self to wait queue
// target end with triggering

// trigger //
// semantic: give target the permission to run, no ordering in current context


}