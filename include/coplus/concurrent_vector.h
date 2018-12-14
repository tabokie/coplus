#pragma once

#include <vector>

namespace coplus {

template <typename>
class ConcurrentVector {

}

template <typename ...Args>
class ConcurrentVector {
	size_t elementSize = sizeof...(Args);
	std::tuple<bool, Args...> *data; // valid bit
	size_t capacity;
	size_t size;
	const int kConcurrentVectorInitSize = 10;
	ConcurrentVector(): ConcurrentVector(kConcurrentVectorInitSize) {}
	ConcurrentVector(size_t init) {
		data = new std::tuple<Args...>(init);
		capacity = init;
		size = 0;
	}
};

} // namespace coplus

