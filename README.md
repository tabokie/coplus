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

