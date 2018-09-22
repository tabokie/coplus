#pragma once

#include <thread>
#include <functional>
#include <memory>
#include <vector>
#include <future>
#include <type_traits>
#include <iostream>
#include <queue>
#include <cassert>


// like kfifo, a lock-free queue for single consumer & producer
// implemented using atomic update of head/tail
// block hardly happened for single consumer/producer context
// may happened for bounded queue and exausted linker
template <typename ElementType>
class SingleStreamQueue {
	std::atomic<bool> in_signal_;
	std::atomic<bool> out_signal_;
	std::vector<ElementType> data_;
	uint32_t radix_;
	uint32_t in_;
	uint32_t out_;
 public:
 	SingleStreamQueue(uint32_t initial = 6, bool bounded = false): data_(2<<initial), radix(initial){ // for bounded, possible to block
 		std::atomic_init(&in_signal_, false);
 		std::atomic_init(&out_signal_, false);
 	}
 	inline bool set_in(void) {
 		bool seted = false;
 		std::atomic_compare_exchange_strong(&in_signal_, &seted, true);
 		return !seted; // if occupied, seted will be set to true
 	}
 	inline void reset_in(void) {
 		std::atomic_store(&in_signal_, false); // set anyway
 		return ;
 	}
 	inline bool set_out(void) {
 		bool seted = false;
 		std::atomic_compare_exchange_strong(&out_signal_, &seted, true);
 		return !seted;
 	}
 	inline void reset_out(void) {
 		std::atomic_store(&out_signal_, false);
 		return ;
 	}
 	inline void push(ElementType& item)
 	inline pop(ElementType item){

 	}
 	// push
 	SingleStreamQueue& operator<<(ElementType item) { // blocking input
 		while(!set_in()) { }
 		item
 		reset_in();
 		return *this;
 	}
 	bool try_push(ElementType item) {
 		if(set_in()){
 			item;
 			reset_in();
 			return true;
 		}
 		return false;
 	}
 	// pop
 	SingleStreamQueue& operator>>(ElementType& item) {
 		while(!set_out()) { }
 		item;
 		reset_out();
 		return *this;
 	}
 	bool try_pop(ElementType& item) {
 		if(set_out()){
 			item = 
 			reset_out();
 			return true;
 		}
 		return false;
 	}
 	// recap
 	bool recap(size_t cap, bool in_set = false, bool out_set = false) {
 		if(!in_set){
 			while(!set_in()){ }
 		}
 		if(!out_set){
 			while(!set_out()){ }
 		}



 		if(!in_set){
 			reset_in();
 		}
 		if(!out_set){
 			reset_out();
 		}
 	}

};


// unordered unlimited queue without sync needed
// maintain thread local order
// provide basic interface of <<, >>, non-blocking
// implementation using thread_local buffer
template <typename ElementType>
class FastQueue{
	std::mutex queues_
	std::vector<std::weak_ptr<std::queue<ElementType>> > local_queues;
	std::queue<ElementType> main_queue;
 public:
 	std::shared_ptr<std::queue<ElementType>> Register(void){
 		auto newQueue = std::make_shared<std::queue<ElementType>>();
 		local_queues.push_back(newQueue); // convert to weak_ptr
 		return newQueue;
 	}
	FastQueue& operator<<(ElementType item){
		thread_local std::shared_ptr<std::queue<ElementType>> local_queue = Register();
		(*local_queue) << item;
		return *this;
	}
	FastQueue& operator>>(ElementType& ret){
		return *this;
	}

};
