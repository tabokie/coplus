# coplus

**coplus** is a parallel programming library written in C++11. Currently, it is planned to supports coroutine based on Windows Fiber, thread pool with dynamic scheduler, and Go-like parallel utilities including Channel.


## Features:

* Scheduler in coplus is mostly based on wait-free structure
* No dependence on third-party library like boost::context

## Todos:
* [x] Message Channel
* [x] Single Thread Pool
* [x] Multi-Thread Pool with locked queue
* [x] Thread Pool with wait-free queue
* [ ] Thread Pool with coroutine
* [ ] Concurrent tools

## Benchmark:

Under 4-core computer, execute enqueue operation with 3 working threads on FastQueue and NaiveQueue(std::queue with mutex), FastQueue outperforms NaiveQueue by 8 times.