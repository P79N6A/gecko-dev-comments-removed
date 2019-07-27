









#ifndef WEBRTC_BASE_ATOMICOPS_H_
#define WEBRTC_BASE_ATOMICOPS_H_

#include <string>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/scoped_ptr.h"

namespace rtc {








template <typename T>
class FixedSizeLockFreeQueue {
 private:

#if defined(__arm__)
  typedef uint32 Atomic32;

  
  static inline void MemoryBarrier() {
    asm volatile("dmb":::"memory");
  }

  
  static inline void AtomicIncrement(volatile Atomic32* ptr) {
    Atomic32 str_success, value;
    asm volatile (
        "1:\n"
        "ldrex  %1, [%2]\n"
        "add    %1, %1, #1\n"
        "strex  %0, %1, [%2]\n"
        "teq    %0, #0\n"
        "bne    1b"
        : "=&r"(str_success), "=&r"(value)
        : "r" (ptr)
        : "cc", "memory");
  }
#elif !defined(SKIP_ATOMIC_CHECK)
#error "No atomic operations defined for the given architecture."
#endif

 public:
  
  FixedSizeLockFreeQueue() : pushed_count_(0),
                             popped_count_(0),
                             capacity_(0),
                             data_() {}
  
  FixedSizeLockFreeQueue(size_t capacity) : pushed_count_(0),
                                            popped_count_(0),
                                            capacity_(capacity),
                                            data_(new T[capacity]) {}

  
  
  
  bool PushBack(T value) {
    if (capacity_ == 0) {
      LOG(LS_WARNING) << "Queue capacity is 0.";
      return false;
    }
    if (IsFull()) {
      return false;
    }

    data_[pushed_count_ % capacity_] = value;
    
    
    MemoryBarrier();
    AtomicIncrement(&pushed_count_);
    return true;
  }

  
  
  
  bool PeekFront(T* value_out) {
    if (capacity_ == 0) {
      LOG(LS_WARNING) << "Queue capacity is 0.";
      return false;
    }
    if (IsEmpty()) {
      return false;
    }

    *value_out = data_[popped_count_ % capacity_];
    return true;
  }

  
  
  
  bool PopFront(T* value_out) {
    if (PeekFront(value_out)) {
      AtomicIncrement(&popped_count_);
      return true;
    }
    return false;
  }

  
  
  void ClearAndResizeUnsafe(int new_capacity) {
    capacity_ = new_capacity;
    data_.reset(new T[new_capacity]);
    pushed_count_ = 0;
    popped_count_ = 0;
  }

  
  int IsFull() const { return pushed_count_ == popped_count_ + capacity_; }
  
  int IsEmpty() const { return pushed_count_ == popped_count_; }
  
  
  size_t Size() const { return pushed_count_ - popped_count_; }

  
  size_t capacity() const { return capacity_; }

 private:
  volatile Atomic32 pushed_count_;
  volatile Atomic32 popped_count_;
  size_t capacity_;
  rtc::scoped_ptr<T[]> data_;
  DISALLOW_COPY_AND_ASSIGN(FixedSizeLockFreeQueue);
};

}

#endif  
