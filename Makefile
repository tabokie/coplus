SRC = src/fiber_task.cc src/fiber_port.cc src/coroutine.cc src/thread_pool.cc src/async_timer.cc src/socket.cc
FLAGS = /EHsc /Iinclude
arena_test:
	cl $(FLAGS) test/arena_test.cc $(SRC)
socket_test:
	cl $(FLAGS) test/client.cc $(SRC)
	cl $(FLAGS) test/server.cc $(SRC)
coroutine_main:
	cl $(FLAGS) test/timer_test.cc $(SRC) src/entry.cc
user_main:
	cl $(FLAGS) test/timer_test.cc $(SRC)