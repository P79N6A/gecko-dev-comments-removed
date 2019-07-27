


















#ifndef WEBRTC_BASE_SCOPED_AUTORELEASE_POOL_H__
#define WEBRTC_BASE_SCOPED_AUTORELEASE_POOL_H__

#if defined(WEBRTC_MAC)

#include "webrtc/base/common.h"


#ifdef __OBJC__
@class NSAutoreleasePool;
#else
class NSAutoreleasePool;
#endif

namespace rtc {

class ScopedAutoreleasePool {
 public:
  ScopedAutoreleasePool();
  ~ScopedAutoreleasePool();

 private:
  
  
  
  
  
  
  void* operator new(size_t size) throw() { return NULL; }
  void operator delete (void* ptr) {}

  NSAutoreleasePool* pool_;

  DISALLOW_EVIL_CONSTRUCTORS(ScopedAutoreleasePool);
};

}  

#endif  
#endif  
