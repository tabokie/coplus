# coplus

**coplus** is a parallel programming library written in C++11. Currently, it is planned to supports coroutine based on Windows Fiber, thread pool with dynamic scheduler, and Go-like parallel utilities including Channel.


## Features

* Scheduler in coplus is mostly based on wait-free structure
* No dependence on third-party library like boost::context

## Todos
* [x] Message Channel
* [x] Single Thread Pool
* [x] Multi-Thread Pool with locked queue
* [x] Thread Pool with wait-free queue
* [ ] Thread Pool with coroutine
* [ ] Concurrent tools: multi-player channel

## Components

### Concurrent Data Structure

* `PushOnlyFastStack`: provide a interface for concurrent data collection with **push** and **retrieve-by-id**.
Also, this structure contains a partial spec for std::unique_ptr which return naked pointer as retrived result.

* `SlowQueue`: filo queue sync using a std mutex, used in low-concurrency condition like freelist in `FastQueue`.

* `FastLocalQueue`: provide a multi-consumer single-producer queue structure, which is wait-free and highly optimized.

* `FastQueue`: a synthesize structure composed of the structures mentioned before. Provide a fully concurrent filo queue structure.

### Non-preemptive Coroutine

Traditional implementation of coroutine includes following interfaces:

* create a coroutine

Each scheduler in ThreadPool is by itself a coroutine. When a task being schedule is a coroutine, the inner call method execute a Switch operation.

* exit a coroutine

To exit a coroutine, we need to know the previous coroutine to go to. To accomplish that, all tasks that is possiblly called as coroutine must take a return coroutine as parameter.

* yield

Simply store the current state and yield to return to return coroutine. A global area is used to store all undone coroutine, and a waiting coroutine must be reinsert into active task queue.

* await

In my implementation, await has many forms:

a) await for funtion
b) await for std conditon variable
c) await for unconditional trigger
d) await for timer

All four of them is in essence accomplished by a `register-notify` mechanism. `Register` receives a waiter's coroutine address, `Notify` switch to one of its waiter or simply re-submit as a task.

## Benchmark

Under 4-core computer, execute enqueue operation with 3 working threads on FastQueue and NaiveQueue(std::queue with mutex), FastQueue outperforms NaiveQueue by 8 times.