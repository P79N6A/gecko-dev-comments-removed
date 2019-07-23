



#ifndef BASE_SCOPED_NSAUTORELEASE_POOL_H_
#define BASE_SCOPED_NSAUTORELEASE_POOL_H_

#include "base/basictypes.h"

#if defined(OS_MACOSX)
#if defined(__OBJC__)
@class NSAutoreleasePool;
#else  
class NSAutoreleasePool;
#endif  
#endif  

namespace base {









class ScopedNSAutoreleasePool {
 public:
#if !defined(OS_MACOSX)
  ScopedNSAutoreleasePool() {}
  void Recycle() { }
#else  
  ScopedNSAutoreleasePool();
  ~ScopedNSAutoreleasePool();

  
  
  
  
  void Recycle();
 private:
  NSAutoreleasePool* autorelease_pool_;
#endif  

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedNSAutoreleasePool);
};

}  

#endif  
