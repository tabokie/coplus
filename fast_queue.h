#ifndef COPLUS_FAST_QUEUE_H_
#define COPLUS_FAST_QUEUE_H_

#include <thread> // thread
#include <functional> // function
#include <memory> // shared_ptr
#include <vector> // vector
#include <future> // std::future
#include <type_traits> // std::enable_if , result_of, is_void
#include <iostream>
#include <queue> // std::queue
#include <cassert> // assert
#include <algorithm> // std::min
#include <cstdint>

#include "corand.h"

namespace coplus{
// Design Overview //
// Top: [Queue list & Free list]
// Queue List (In Stack, wait-free)
// - GetQueue(token)
// - AddQueue
// - Compact(GC) // deprecated for use of handle
// Free List (In-Out Stack, lock)
// - GetFree
// - AddFree
// Bottom: [Single-Producer Multi-Comsumer Queue]
// - try_push
// - try_push_hard
// - try_pop

template <typename ElementType, size_t unitSize = 10>
class PushOnlyFastStack{
	std::vector<std::atomic<ElementType*>> stacks;
	std::atomic<int> end; // point to the first empty slot, meaning size
	int max_size;
 public:
 	PushOnlyFastStack(size_t maxSize): stacks((maxSize+unitSize-1) / unitSize, nullptr), max_size(maxSize) {
 		std::atomic_init(&end, 0);
 	}
	// get element
	bool Get(int token, ElementType& ret){
		if(token > max_size)return -1;
		int mainSlot = token / unitSize;
		int subSlot = token - mainSlot * unitSize;
		ElementType* oldStack;
		if((oldStack = std::atomic_load(stacks[mainSlot])) == nullptr){
			return false;
		}
		else{
			ret = oldStack[subSlot];
			return true;
		}
	}
	// return new token
	int Add(ElementType&& newItem){
		int slot = std::atomic_fetch_add(&end, 1);
		if(slot > max_size)return -1;
		int mainSlot = slot / unitSize;
		int subSlot = slot - mainSlot * unitSize;
		ElementType* oldStack;
		if((oldStack = std::atomic_load(stacks[mainSlot])) == nullptr){
			ElementType* newStack = new ElementType[unitSize];
			if(std::atomic_compare_exchange_strong(&stacks[mainSlot], &oldStack, newStack)){
				// first to new
				oldStack = newStack;
			}
			else{
				delete [] newStack;
			}
		}
		oldStack[subSlot] = std::move(newItem);
		return slot;
	}

};

template <typename ElementType>
class FastStack{
	int max_size;
	std::vector<ElementType> ring;
	int head, tail; // head pop if < tail, tail push
	std::mutex lock;
 public:
 	// only head=tail=0 is empty, other is full
 	FastStack(int maxSize): max_size(maxSize), head(0), tail(0), ring(max_size) {	}
 	bool Get(ElementType& ret){ // get and delete
 		std::lock_guard<std::mutex> local(lock);
 		if(head == tail && tail == 0){
 			return false;
 		}
 		int slot = head;
 		head = (head==max_size-1) ? 0 : head+1;
 		if(head == tail){
 			head = tail = 0; // empty
 		}
 		return ring[slot];
 	}
 	bool Add(ElementType item){
 		std::lock_guard<std::mutex> local(lock);
 		if(head == tail){
 			if(head == 0){
 				tail ++;
 				ring[0] = item;
 				return true;
 			}
 			else{
 				return false;
 			}
 		}
 		ring[tail] = item;
 		tail = (tail==max_size-1) ? 0 : tail+1;
 		if(head == tail && tail == 0){
 			head = tail = 1; // no equal 0
 		}
 	}
};

template <typename ElementType>
struct FastQueueBuffer: {
	ElementType* data;
	size_t size; // elements
	FastQueueBuffer(size_t size): size(size){
		data = new ElementType[size];
	}
	~FastQueueBuffer(){delete [] data;}
	void recap(size_t newSize, bool copy = true){
		ElementType* newData = new ElementType[newSize];
		if(!newData)
		if(copy)memcpy((void*)newData, (void*)data, sizeof(ElementType) * std::min(size, newSize));
		size = newSize;
		delete [] data;
		data = newData;
		return ;
	}
	ElementType& operator[](size_t index){
		return data[index];
	}
};

template <typename ElementType>
class FastLocalQueue{
	// not implemented
};

// unordered unlimited queue without sync needed
// maintain thread local order
// provide basic interface of <<, >>, non-blocking
// implementation using thread_local buffer
template <typename ElementType>
class FastQueue{
	using Token = uint32_t;
	PushOnlyFastStack<std::unique_ptr(FastLocalQueue<ElementType>)> QueueList;
	FastStack<Token> FreeList;
 public:
 	// for a non-handle producer
 	// push to free listed queue

 	// second order probing
 	// no recap allowed
 	bool try_push(Token token, ElementType item){
 		corand.UInt()
 	}
 	// try to recap
 	bool try_push_hard(Token token, ElementType item){

 	}
 	bool try_push(ElementType item){ // immutable global queue

 	}
 	// pop fail if approximate empty
 	bool try_pop(ElementType& ret){

 	} 	

 	// for handle producer
	// when delete a handle, queue freed only for new producer
	// consumer still keep probing
 	struct Handle{ 
 		Token token;
		FastQueueProducerHandle(FastQueue<ElementType>* queue): queue_(queue){
			// create a local queue here
		}
		~FastQueueProducerHandle(){
			// free or delete the local queue here
		}
	};
	
	// free list is compact when explicitly called
	// only free empty and free queues

};





// Old implementation with mutex like atomic flag
/*
// like kfifo, a lock-free queue for single consumer & producer
// implemented using atomic update of head/tail
// block hardly happened for single consumer/producer context
// may happened for bounded queue and exausted linker
template <typename ElementType>
class SingleStreamQueue {
	std::atomic<bool> in_signal_;
	std::atomic<bool> out_signal_;
	ElementType* data_;
	uint32_t radix_;
	bool recap_;
	uint32_t in_; // in_ is free to push
	uint32_t out_; // out_ is free to pop if in>out
 public:
 	SingleStreamQueue(uint32_t initial = 6, bool bounded = false)
 		: radix_(initial), recap_(!bounded){ // for bounded, possible to block
 		data_ = new ElementType[2 << radix_];
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
 	// full -> recap / empty -> 
 	inline void push(ElementType item) {

 	}
 	inline void pop(ElementType& item){

 	}
 	// push
 	SingleStreamQueue& operator<<(ElementType item) { // blocking input
 		while(!set_in()) { }
 		while(in_ == out_ && recap_){
 			recap(radix_+1, true);
 		}
 		uint32_t in = in_ & (0xffffffff << radix_);

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
 		if()
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

 		ElementType* newData = new ElementType[2 << cap];
 		memcpy(newData, data_+out_, std::min(in_-out_, (2<<radix_)-out_) * sizeof(ElementType));
 		if()
 		memcpy(newData)
 		if(!in_set){
 			reset_in();
 		}
 		if(!out_set){
 			reset_out();
 		}
 	}

};
*/	

} // namespace coplus


#endif // COPLUS_FAST_QUEUE_H_