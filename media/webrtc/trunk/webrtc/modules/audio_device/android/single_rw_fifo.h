









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_SINGLE_RW_FIFO_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_SINGLE_RW_FIFO_H_

#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {





class SingleRwFifo {
 public:
  explicit SingleRwFifo(int capacity);
  ~SingleRwFifo();

  void Push(int8_t* mem);
  int8_t* Pop();

  void Clear();

  int size() { return size_.Value(); }
  int capacity() const { return capacity_; }

 private:
  scoped_array<int8_t*> queue_;
  int capacity_;

  Atomic32 size_;

  int read_pos_;
  int write_pos_;
};

}  

#endif  
