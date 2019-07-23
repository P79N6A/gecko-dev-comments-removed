



































#ifndef MacAutoreleasePool_h_
#define MacAutoreleasePool_h_


#ifdef __OBJC__
@class NSAutoreleasePool;
#else
class NSAutoreleasePool;
#endif

namespace mozilla {

class MacAutoreleasePool {
public:
  MacAutoreleasePool();
  ~MacAutoreleasePool();

private:
  NSAutoreleasePool *mPool;

  MacAutoreleasePool(const MacAutoreleasePool&);
  void operator=(const MacAutoreleasePool&);
};

}

#endif 
