




































#ifndef PCOMContentPermissionRequestChild_h
#define PCOMContentPermissionRequestChild_h

#include "mozilla/dom/PContentPermissionRequestChild.h"

#undef CreateEvent










class PCOMContentPermissionRequestChild : public mozilla::dom::PContentPermissionRequestChild {
public:
  virtual void IPDLRelease() = 0;
};

#endif
