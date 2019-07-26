









#include "webrtc/modules/audio_device/android/single_rw_fifo.h"

#if !defined(__ARMEL__)

#error trying to compile ARM code for non-ARM target
#endif

static int UpdatePos(int pos, int capacity) {
  return (pos + 1) % capacity;
}

namespace webrtc {

namespace subtle {





inline void MemoryBarrier() {
  
  
  typedef void (*KernelMemoryBarrierFunc)();
  ((KernelMemoryBarrierFunc)0xffff0fa0)();
}

}  

SingleRwFifo::SingleRwFifo(int capacity)
    : capacity_(capacity),
      size_(0),
      read_pos_(0),
      write_pos_(0) {
  queue_.reset(new int8_t*[capacity_]);
}

SingleRwFifo::~SingleRwFifo() {
}

void SingleRwFifo::Push(int8_t* mem) {
  assert(mem);

  
  
  
  const int free_slots = capacity() - size();
  if (free_slots <= 0) {
    
    
    assert(false);
    return;
  }
  queue_[write_pos_] = mem;
  
  subtle::MemoryBarrier();
  ++size_;
  write_pos_ = UpdatePos(write_pos_, capacity());
}

int8_t* SingleRwFifo::Pop() {
  int8_t* ret_val = NULL;
  if (size() <= 0) {
    
    
    assert(false);
    return ret_val;
  }
  ret_val = queue_[read_pos_];
  
  subtle::MemoryBarrier();
  --size_;
  read_pos_ = UpdatePos(read_pos_, capacity());
  return ret_val;
}

}  
