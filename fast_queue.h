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
// Free List (In-Out Stack, non-busy structure use lock)
// - GetFree
// - AddFree
// Bottom: [Single-Producer Multi-Comsumer Queue]
// - try_push
// - try_push_hard
// - try_pop
// Procedure //
// Produce: Ask Handle -> Ask Free Queue -> Fetch or Create
// -> Produce -> Full and Handle Transfer(need send in handle) -> Ask Free Queue and Free before
// Consume: Ask Port -> Random and Probe (hard probe serially, weak probe 2-order)

template <typename ElementType, size_t unitSize = 10>
class PushOnlyFastStack{
	std::atomic<ElementType*>* stacks;
	std::atomic<int> end; // point to the first empty slot, meaning size
	size_t max_size;
 public:
 	PushOnlyFastStack(size_t maxSize): max_size(maxSize){
 		stacks = new std::atomic<ElementType*>[(maxSize+unitSize-1) / unitSize];
 		for(int i = 0; i < (maxSize+unitSize-1) / unitSize; i++)
 			stacks[i].store((ElementType*)NULL);
 		end.store(0);
 	}
	~PushOnlyFastStack(){
		for(int i = 0; i < (max_size+unitSize-1) / unitSize; i++){
			delete [] stacks[i].load();
		}
		delete [] stacks;
	}
	// get element, no delete
	bool get(int token, ElementType& ret){
		if(token > max_size)return false;
		int mainSlot = token / unitSize;
		int subSlot = token - mainSlot * unitSize;
		ElementType* oldStack;
		if((oldStack = stacks[mainSlot].load()) == NULL){
			return false;
		}
		else{
			ret = oldStack[subSlot];
			return true;
		}
	}
	// return new token
	int push(ElementType newItem){
		int slot = std::atomic_fetch_add(&end, 1) ; // start from 0
		if(slot >= max_size)return -1;
		int mainSlot = slot / unitSize;
		int subSlot = slot - mainSlot * unitSize;
		ElementType* oldStack;
		if((oldStack = stacks[mainSlot].load()) == NULL){
			ElementType* newStack = new ElementType[unitSize];
			if(std::atomic_compare_exchange_strong(&stacks[mainSlot], &oldStack, newStack)){
				// first to new
				oldStack = newStack;
			}
			else{
				delete [] newStack;
			}
		}
		oldStack[subSlot] = (newItem);
		return slot;
	}

	int size(void){
		return end.load();
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
 	FastStack(int maxSize): max_size(maxSize), head(0), tail(0), ring(maxSize) {	}
 	bool pop(ElementType& ret){ // get and delete
 		std::lock_guard<std::mutex> local(lock);
 		if(head == tail && tail == 0){
 			return false;
 		}
 		int slot = head;
 		head = (head==max_size-1) ? 0 : head+1;
 		if(head == tail){
 			head = tail = 0; // empty
 		}
 		ret = ring[slot];
 		return true;
 	}
 	bool push(ElementType item){
 		std::lock_guard<std::mutex> local(lock);
 		if(head == tail){
 			if(head == 0){
 				tail ++;
 				ring[0] = (item);
 				return true;
 			}
 			else{
 				return false;
 			}
 		}
 		ring[tail] = (item);
 		tail = (tail==max_size-1) ? 0 : tail+1;
 		if(head == tail && tail == 0){
 			head = tail = 1; // no equal 0
 		}
 		return true;
 	}
 	size_t size(void){
 		std::lock_guard<std::mutex> local(lock);
 		if(head == tail && tail == 0)return 0;
 		if(head == tail)return max_size;
 		if(head > tail)return tail - head + max_size;
 		return tail - head;
 	}
};

// Push: atomic add push count, if push-pop < QueueSize, pass; else add push rependent count
// Pop: atomic add pop count, if push-pop > 0, pass; else 
template <typename ElementType, size_t QueueSize = 20>
class FastLocalQueue{
	// outer layer
	std::atomic<int> enqueue_count_;
	std::atomic<int> enqueue_revoke_;
	std::atomic<int> dequeue_count_;
	std::atomic<int> dequeue_revoke_;
	// inner layer
	std::atomic<uint32_t> head_;
	std::atomic<uint32_t> tail_;
	// data layer
	ElementType data_[QueueSize];
 public:
	FastLocalQueue(): head_(0), tail_(0), enqueue_count_(0), dequeue_count_(0), enqueue_revoke_(0), dequeue_revoke_(0) { }
	~FastLocalQueue() { }
	bool push(ElementType item){
		// first check outer layer
		int enq = std::atomic_fetch_add(&enqueue_count_, 1) + 1;
		if(enq - enqueue_revoke_.load() - dequeue_count_.load() + dequeue_revoke_.load() > QueueSize){
			std::atomic_fetch_add(&enqueue_revoke_, 1);
			return false;
		}
		data_[std::atomic_fetch_add(&tail_, 1)] = (item);
		return true;
	}
	bool pop(ElementType& ret){
		// first check outer layer
		int deq = std::atomic_fetch_add(&dequeue_count_, 1) + 1;
		if(enqueue_count_.load() - enqueue_revoke_.load() - deq + dequeue_revoke_.load() < 0){
			std::atomic_fetch_add(&dequeue_revoke_, 1);
			return false;
		}
		ret = data_[std::atomic_fetch_add(&head_, 1)];
		return true;
	}
	size_t size(void){
		int current = tail_.load() - head_.load();
		if(current == 0){
			// rough assumption of concurrency
			if( (enqueue_count_.load() - enqueue_revoke_.load() - dequeue_count_.load() + dequeue_revoke_.load()) > QueueSize / 2){
				current = tail_.load() - head_.load();
				return (current == 0) ? QueueSize : current;
			}
			else{
				return tail_.load() - head_.load();
			}
		}
		return current;
	}
	void log(void){
		colog.put_batch(enqueue_count_.load(),
			enqueue_revoke_.load(),dequeue_count_.load(),dequeue_revoke_.load(),head_.load(),tail_.load());
	}

};

// 1. fixed local size
// 2. shared_ptr overhead
// 3. Handle delegate
constexpr size_t kLocalQueueSize = 20;
constexpr size_t kFreeListSize = 50;
constexpr size_t kQueueMaxCount = 100;
// unordered unlimited queue without sync needed
// maintain thread local order
// provide basic interface of <<, >>, non-blocking
// implementation using thread_local buffer
template <typename ElementType>
class FastQueue{
	using Token = int;
 	using LocalQueuePtr = std::shared_ptr<FastLocalQueue<ElementType, kLocalQueueSize>>;
	PushOnlyFastStack<LocalQueuePtr> QueueList;
	FastStack<Token> FreeList;
 public:
 	FastQueue(): FreeList(kFreeListSize), QueueList(kQueueMaxCount) { }
 	bool create_queue(Token& newToken){
 		auto token = QueueList.push(std::make_shared<FastLocalQueue<ElementType, kLocalQueueSize>>());
 		if(token < 0)return false;
 		newToken = token;
 		return true;
 	}
 	// for handle producer
	// when delete a handle, queue freed only for new producer
	// consumer still keep probing
 	struct ProducerHandle{
 		const int kProducerRetry = 10;
 		Token token;
 		FastQueue<ElementType>* queue_;
 		LocalQueuePtr local_; // no one is to delete queue, use shared
		ProducerHandle(FastQueue<ElementType>* queue): queue_(queue), local_(nullptr){
			// create a local queue here
			int retry = kProducerRetry;
			while(--retry && !apply()) { }
			if(retry == 0){
				retry = kProducerRetry;
				while(--retry && !preempt()) { }
			}
		}
		bool apply(void){
			bool ret = queue_->FreeList.pop(token);
			if(!ret)return false;
			local_ = nullptr;
			ret = queue_->QueueList.get(token, local_);
			return ret;
		}
		bool preempt(void){
			bool ret = queue_->create_queue(token);
			if(!ret)return false;
			local_ = nullptr;
			ret = queue_->QueueList.get(token, local_);
			return ret;
		}
		bool push(ElementType&& item){
			if(!local_) return false; // no retry
			return local_->push(item);
		}
		bool push_hard(ElementType&& item){ // skip to new local queue
			int retry;
			if(!local_){
				retry = kProducerRetry;
				while(--retry && !apply()) {  } 
				if(retry == 0){
					retry = kProducerRetry;
					while(--retry && !preempt()) {  }
				}
			}
			if(!local_)return false;
			
			retry = kProducerRetry;
			while(--retry && !local_->push(item)) {  }
			if(retry == 0){
				
				queue_->FreeList.push(token);
				retry = kProducerRetry;
				while(--retry && !preempt()) {  }
				if(!local_)return false;
			}
			
			retry = kProducerRetry;
			while(--retry && !local_->push(item)) {  }
			return retry != 0;
		}
		~ProducerHandle(){
			// free or delete the local queue here
			if(local_){
				queue_->FreeList.push(token);
			}
		}
	};
 	// pop fail if approximate empty
 	bool pop(ElementType& ret){
 		// randomly choose a local
 		int index = corand.UInt(QueueList.size());
 		int cur = index;
 		LocalQueuePtr local = nullptr;
 		bool status;
 		for(int i = 0; i==0 || cur != index; i++, cur += 2*i+1){
 			cur = cur % QueueList.size();
 			status = QueueList.get(cur, local);
 			if(!status || !local)return false;
 			status = local->pop(ret);
 			if(status)return true;
 		}
 		return false;
 	}
 	// probing in sequential way
 	bool pop_hard(ElementType& ret){
 		LocalQueuePtr local = nullptr;
 		bool status;
 		int approximateSize = QueueList.size();
 		for(int index = 0; index < approximateSize; index ++){
 			status = QueueList.get(index, local);
 			if(!status || !local)return false;
 			status = local->pop(ret);
 			if(status)return true;
 			if(index == approximateSize - 1){
 				approximateSize = QueueList.size(); // update size at end
 			}
 		}
 		return false;
 	}
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