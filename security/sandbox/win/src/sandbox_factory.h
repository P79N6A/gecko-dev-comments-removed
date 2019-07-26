



#ifndef SANDBOX_SRC_SANDBOX_FACTORY_H__
#define SANDBOX_SRC_SANDBOX_FACTORY_H__

#include "sandbox/win/src/sandbox.h"
























namespace sandbox {

class SandboxFactory {
 public:
  
  
  static BrokerServices* GetBrokerServices();

  
  
  static TargetServices* GetTargetServices();
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SandboxFactory);
};

}  

#endif  
