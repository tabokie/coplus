# coplus

**coplus** is a parallel programming library written in C++11. Currently, it is planned to supports coroutine based on Windows Fiber, thread pool with dynamic scheduler, and Go-like parallel utilities including Channel.


## Features

* Scheduler in coplus is mostly based on wait-free structure
* No dependence on third-party library like boost::context

## Todos
* [x] Message Channel
* [x] Many-to-one thread pool
* [x] Many-to-many thread pool with lock-sync queue
* [x] Thread pool with wait-free queue
* [x] Thread pool with coroutine
* [ ] Concurrent tools

## Components

### Thread Pool

Targeted at light-weight task scheduling, ThreadPool provides simple interface for complex concurrent controls: 

*  `Machine`: Runs scheduler routine on user thread, easy to `close` and `restart`.
*  `Task` : Provides base interface for Schedulable job, easy to implement with `bool call()`, `void raw_call()`.
*  `ThreadPool` : Includes `submit`, `resize`, `close`, `report` functions.


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
  // Use Case with returned function
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

```c++
  // Use Case
  auto trace = go(()-> int{
    int count = 100;
    while(count--){
      auto trace = await([]{return 7;});
      colog << trace.get(); // no blocking here
    }
    return 42;
  });
  colog << trace.get();
```

​	 **b)** await for unconditional trigger

```c++
  // Use Case
  Trigger lock = NewTrigger;
  int data = -1;
  auto trace = go([&]()-> int{
    while(true){
      await(lock);
      // need sync if more task notify lock
      if(data > 0)break;
    }
    return data;
  });
  go([&]{
    data = 7;
    notify(lock);
    return ;
  });
  colog << trace.get(); // should be 7 here;
```

​	 **c)** await for timer

All four of them are in essence accomplished by a `register-notify` mechanism. `Register` receives a waiter's 			coroutine address, `Notify` switch to one of its waiters or simply re-submit them as tasks.

### Concurrent Data Structure

- `PushOnlyFastStack`: Provide a interface for concurrent data collection with **push** and **retrieve-by-id**.
  Also, This structure contains a partial spec for std::unique_ptr which return naked pointer as retrived result.
- `SlowQueue`: Filo queue sync using a std mutex, used in low-concurrency condition like freelist in `FastQueue`.
- `FastLocalQueue`: Provide a multi-consumer single-producer queue structure, which is wait-free and highly optimized.
- `FastQueue`: A synthesize structure composed of the structures mentioned before. Provide a fully concurrent filo queue structure.

### Concurrent Tools

* `Channel`: Message transport tool mocking Go's chan. Current implementation uses std mutex and condition_variable to provide basic function, further optimization will introduce lock-free shared memory.
* `colog`: Message output tools merely for debug or strong ordering condition, synchronized with plain lock. Enhanced implementation could uses thread local buffer to increase throughput, or `FastQueue` to avoid blocking. 
* `cotimer`: Thread local timer.
* `Socket`

## Benchmark

Under 4-core computer, execute enqueue operation with 3 working threads on FastQueue and NaiveQueue(std::queue with mutex), FastQueue outperforms NaiveQueue by 8 times.