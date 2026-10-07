CXX ?= g++
CPPFLAGS += -I.
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2

.PHONY: all test demo sanitize clean
all: build/test_indpq build/task_scheduler

build:
	mkdir -p build

build/test_indpq: tests/test_indpq.cpp IndPQ.h | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

build/task_scheduler: examples/task_scheduler.cpp IndPQ.h | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

test: build/test_indpq
	./build/test_indpq

demo: build/task_scheduler
	./build/task_scheduler

sanitize: | build
	$(CXX) $(CPPFLAGS) -std=c++17 -Wall -Wextra -Wpedantic -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_indpq.cpp -o build/test_sanitize
	./build/test_sanitize

clean:
	rm -rf build
