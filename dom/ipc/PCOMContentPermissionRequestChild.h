




































#include "mozilla/dom/PContentPermissionRequestChild.h"

#undef CreateEvent










class PCOMContentPermissionRequestChild : public mozilla::dom::PContentPermissionRequestChild {
public:
  virtual void IPDLRelease() = 0;
};
