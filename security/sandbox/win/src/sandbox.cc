



#include <stdio.h>
#include <windows.h>
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/broker_services.h"
#include "sandbox/win/src/target_services.h"

namespace sandbox {

SANDBOX_INTERCEPT HANDLE  g_shared_section = NULL;

static bool               s_is_broker =  false;



BrokerServices* SandboxFactory::GetBrokerServices() {
  
  if (NULL != g_shared_section) {
    return NULL;
  }
  
  
  s_is_broker = true;
  return BrokerServicesBase::GetInstance();
}



TargetServices* SandboxFactory::GetTargetServices() {
  
  if (NULL == g_shared_section) {
    return NULL;
  }
  
  s_is_broker = false;
  
  return TargetServicesBase::GetInstance();
}

}  
