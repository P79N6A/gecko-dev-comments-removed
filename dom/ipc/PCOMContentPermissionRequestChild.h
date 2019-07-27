



#ifndef PCOMContentPermissionRequestChild_h
#define PCOMContentPermissionRequestChild_h

#include "mozilla/dom/PContentPermissionRequestChild.h"



#undef CreateEvent
#undef LoadImage










class PCOMContentPermissionRequestChild : public mozilla::dom::PContentPermissionRequestChild {
public:
  virtual void IPDLRelease() = 0;
#ifdef DEBUG
  PCOMContentPermissionRequestChild() : mIPCOpen(false) {}
  virtual ~PCOMContentPermissionRequestChild() {
    
    
    MOZ_ASSERT(!mIPCOpen, "Protocol must not be open when PCOMContentPermissionRequestChild is destroyed.");
  }
  bool mIPCOpen;
#endif 
};

#endif
