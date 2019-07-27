



#ifndef SANDBOX_SRC_POLICY_BROKER_H_
#define SANDBOX_SRC_POLICY_BROKER_H_

#include "sandbox/win/src/interception.h"

namespace sandbox {

class TargetProcess;


bool SetupBasicInterceptions(InterceptionManager* manager);



bool SetupNtdllImports(TargetProcess *child);

}  

#endif  
