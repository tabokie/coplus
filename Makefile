SRC = src/fiber_task.cc src/fiber_port.cc src/coroutine.cc src/thread_pool.cc src/entry.cc

all:
	cl /Iinclude/ test/coroutine_test.cc $(SRC)