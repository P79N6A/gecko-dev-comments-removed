



#ifndef CHROME_COMMON_TRANSPORT_DIB_H_
#define CHROME_COMMON_TRANSPORT_DIB_H_

#include "base/basictypes.h"

#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_BSD)
#include "base/shared_memory.h"
#endif

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_LINUX)
#include "chrome/common/x11_util.h"
#endif






class TransportDIB {
 public:
  ~TransportDIB();

  
  
  
  
#if defined(OS_WIN)
  typedef HANDLE Handle;
  
  
  
  
  
  
  
  
  struct HandleAndSequenceNum {
    HandleAndSequenceNum()
        : handle(NULL),
          sequence_num(0) {
    }

    HandleAndSequenceNum(HANDLE h, uint32_t seq_num)
        : handle(h),
          sequence_num(seq_num) {
    }

    bool operator< (const HandleAndSequenceNum& other) const {
      
      if (other.handle != handle)
        return other.handle < handle;
      return other.sequence_num < sequence_num;
    }

    HANDLE handle;
    uint32_t sequence_num;
  };
  typedef HandleAndSequenceNum Id;
#elif defined(OS_MACOSX) || defined(OS_BSD)
  typedef base::SharedMemoryHandle Handle;
  
  typedef base::SharedMemoryId Id;
#elif defined(OS_LINUX)
  typedef int Handle;  
  typedef int Id;
#endif

  
  
  
  
  static TransportDIB* Create(size_t size, uint32_t sequence_num);

  
  static TransportDIB* Map(Handle transport_dib);

  
  void* memory() const;

  
  
  
  size_t size() const { return size_; }

  
  
  Id id() const;

  
  
  Handle handle() const;

#if defined(OS_LINUX)
  
  
  XID MapToX(Display* connection);
#endif

 private:
  TransportDIB();
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_BSD)
  explicit TransportDIB(base::SharedMemoryHandle dib);
  base::SharedMemory shared_memory_;
#elif defined(OS_LINUX)
  int key_;  
  void* address_;  
  XID x_shm_;  
  Display* display_;  
#endif
#ifdef OS_WIN
  uint32_t sequence_num_;
#endif
  size_t size_;  
};

class MessageLoop;

#endif  
