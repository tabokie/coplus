SRC = src/fiber_task.cc src/fiber_port.cc src/coroutine.cc src/thread_pool.cc
FLAGS = /EHsc /Iinclude
all:
	cl $(FLAGS) test/coroutine_test.cc $(SRC) src/entry.cc
user_main:
	cl $(FLAGS) test/coroutine_test.cc $(SRC)