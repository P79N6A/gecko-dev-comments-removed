









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SHARED_MEMORY_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SHARED_MEMORY_H_

#include <stddef.h>

#if defined(WEBRTC_WIN)
#include <windows.h>
#endif

#include "webrtc/typedefs.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {







class SharedMemory {
 public:
#if defined(WEBRTC_WIN)
  typedef HANDLE Handle;
  static const Handle kInvalidHandle;
#else
  typedef int Handle;
  static const Handle kInvalidHandle;
#endif

  void* data() const { return data_; }
  size_t size() const { return size_; }

  
  Handle handle() const { return handle_; }

  
  
  int id() const { return id_; }

  virtual ~SharedMemory() {}

 protected:
  SharedMemory(void* data, size_t size, Handle handle, int id);

  void* const data_;
  const size_t size_;
  const Handle handle_;
  const int id_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SharedMemory);
};

}  

#endif  

