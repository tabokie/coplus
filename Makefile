SRC = src/fiber_task.cc src/fiber_port.cc src/coroutine.cc src/thread_pool.cc src/async_timer.cc
FLAGS = /EHsc /Iinclude
all:
	cl $(FLAGS) test/timer_test.cc $(SRC) src/entry.cc
user_main:
	cl $(FLAGS) test/timer_test.cc $(SRC)