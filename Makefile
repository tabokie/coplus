SRC = src/fiber_task.cc src/fiber_port.cc src/coroutine.cc src/thread_pool.cc src/async_timer.cc src/socket.cc
FLAGS = /EHsc /Iinclude
monitor_test:
	cl $(FLAGS) test/monitor_test.cc $(SRC) src/entry.cc
socket_test:
	cl $(FLAGS) test/client.cc $(SRC)
	cl $(FLAGS) test/server.cc $(SRC)
coroutine_main:
	cl $(FLAGS) test/timer_test.cc $(SRC) src/entry.cc
user_main:
	cl $(FLAGS) test/timer_test.cc $(SRC)