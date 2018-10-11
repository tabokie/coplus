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

create_coroutine
destroy_coroutine
switch_coroutine
yield
notify(condv)
await_for(coroutine)
await_for(condv)
await_for(time)

## Benchmark

Under 4-core computer, execute enqueue operation with 3 working threads on FastQueue and NaiveQueue(std::queue with mutex), FastQueue outperforms NaiveQueue by 8 times.