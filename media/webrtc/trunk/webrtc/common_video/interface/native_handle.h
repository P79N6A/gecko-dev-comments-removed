









#ifndef COMMON_VIDEO_INTERFACE_NATIVEHANDLE_H_
#define COMMON_VIDEO_INTERFACE_NATIVEHANDLE_H_

#include "webrtc/typedefs.h"

namespace webrtc {






class NativeHandle {
 public:
  virtual ~NativeHandle() {}
  
  virtual int32_t AddRef() = 0;
  virtual int32_t Release() = 0;

  
  virtual void* GetHandle() = 0;
};

}  

#endif  
