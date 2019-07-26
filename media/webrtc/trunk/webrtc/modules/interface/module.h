









#ifndef MODULES_INTERFACE_MODULE_H_
#define MODULES_INTERFACE_MODULE_H_

#include <assert.h>

#include "typedefs.h"

namespace webrtc {

class Module {
 public:
  
  
  virtual int32_t ChangeUniqueId(const int32_t id) { return 0; }

  
  
  virtual int32_t TimeUntilNextProcess() = 0;

  
  virtual int32_t Process() = 0;

 protected:
  virtual ~Module() {}
};


class RefCountedModule : public Module {
 public:
  
  
  
  
  virtual int32_t AddRef() {
    assert(false && "Not implemented.");
    return 1;
  }

  
  
  
  
  
  
  virtual int32_t Release() {
    assert(false && "Not implemented.");
    return 1;
  }

 protected:
  virtual ~RefCountedModule() {}
};

}  

#endif  
