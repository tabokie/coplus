# coplus

**coplus** is a parallel programming library written in C++11. Currently, it is planned to supports coroutine based on Windows Fiber, thread pool with dynamic scheduler, and Go-like parallel utilities including Channel.


## Features

* Scheduler in coplus is mostly based on wait-free structure
* No dependence on third-party library like boost::context

## Todos
* [x] Message Channel
* [x] Many-to-one thread Pool
* [x] Many-to-many thread Pool with lock-sync queue
* [x] Thread Pool with wait-free queue
* [ ] Thread Pool with coroutine
* [ ] Concurrent tools

## Components

### Concurrent Data Structure

* `PushOnlyFastStack`: provide a interface for concurrent data collection with **push** and **retrieve-by-id**.
Also, this structure contains a partial spec for std::unique_ptr which return naked pointer as retrived result.

* `SlowQueue`: filo queue sync using a std mutex, used in low-concurrency condition like freelist in `FastQueue`.

* `FastLocalQueue`: provide a multi-consumer single-producer queue structure, which is wait-free and highly optimized.

* `FastQueue`: a synthesize structure composed of the structures mentioned before. Provide a fully concurrent filo queue structure.

### Non-preemptive Coroutine

Traditional implementation of coroutine includes following interfaces:

* **create a coroutine**

  Each scheduler in ThreadPool is by itself a coroutine. When a task being schedule is a coroutine, the inner call method execute a Switch operation.

* **exit a coroutine**

  To exit a coroutine, we need to know the previous coroutine to go to. To accomplish that, all tasks that is possibly called as coroutine must take a return coroutine as parameter.

* **yield**

  Simply store the current state and yield to return to return coroutine. A global area is used to store all undone coroutine, and a waiting coroutine must be re-insert into active task queue.

```c++
	// Use Case with void function:
	ThreadPool pool;
	pool.go(std::function<void(void)>([&](){
	  	colog << "In Loop Visit";
	  	int count = 100;
	  	while(count --)
	  		yield();
	  	colog << "Out Loop Visit";
	  	return;
	  }));
  // User Case with returned function
	auto trace = pool.go(std::bind([&](int ret)-> int{
	  	colog << "In Loop Visit";
	  	int count = 100;
	  	while(count --)
	  		yield();
	  	colog << "Out Loop Visit";
	  	status = true;
	  	return ret;
	  }, 42));
  trace.get(); // expect 42
```

* **await**

  In my implementation, await has many forms:

  **a)** await for function

  **b)** await for std condition variable

  **c)** await for unconditional trigger

  **d)** await for timer

  All four of them are in essence accomplished by a `register-notify` mechanism. `Register` receives a waiter's coroutine address, `Notify` switch to one of its waiters or simply re-submit them as tasks.

## Benchmark

Under 4-core computer, execute enqueue operation with 3 working threads on FastQueue and NaiveQueue(std::queue with mutex), FastQueue outperforms NaiveQueue by 8 times.