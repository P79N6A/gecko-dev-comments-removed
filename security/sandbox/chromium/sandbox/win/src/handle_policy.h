



#ifndef SANDBOX_SRC_HANDLE_POLICY_H_
#define SANDBOX_SRC_HANDLE_POLICY_H_

#include <string>

#include "base/basictypes.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/sandbox_policy.h"

namespace sandbox {

enum EvalResult;


class HandlePolicy {
 public:
  
  
  static bool GenerateRules(const wchar_t* type_name,
                            TargetPolicy::Semantics semantics,
                            LowLevelPolicy* policy);

  
  static DWORD DuplicateHandleProxyAction(EvalResult eval_result,
                                          HANDLE source_handle,
                                          DWORD target_process_id,
                                          HANDLE* target_handle,
                                          DWORD desired_access,
                                          DWORD options);
};

}  

#endif  

