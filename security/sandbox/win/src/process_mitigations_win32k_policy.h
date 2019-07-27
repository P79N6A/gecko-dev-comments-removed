



#ifndef SANDBOX_SRC_PROCESS_MITIGATIONS_WIN32K_POLICY_H_
#define SANDBOX_SRC_PROCESS_MITIGATIONS_WIN32K_POLICY_H_

#include "base/basictypes.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/sandbox_policy.h"

namespace sandbox {

enum EvalResult;



class ProcessMitigationsWin32KLockdownPolicy {
 public:
  
  
  
  
  
  static bool GenerateRules(const wchar_t* name,
                            TargetPolicy::Semantics semantics,
                            LowLevelPolicy* policy);
};

}  

#endif  


