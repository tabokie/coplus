SRC = src/fiber_task.cc src/fiber_port.cc src/coroutine.cc src/thread_pool.cc src/async_timer.cc src/socket.cc
FLAGS = /EHsc /Iinclude
socket_test:
	cl $(FLAGS) test/socket_test.cc $(SRC)
	cl test/socket_test_client.cc
coroutine_main:
	cl $(FLAGS) test/timer_test.cc $(SRC) src/entry.cc
user_main:
	cl $(FLAGS) test/timer_test.cc $(SRC)