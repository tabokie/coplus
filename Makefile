SRC = src/fiber_task.cc src/fiber_port.cc

all:
	cl /Iinclude/ test/coroutine_test.cc $(SRC)