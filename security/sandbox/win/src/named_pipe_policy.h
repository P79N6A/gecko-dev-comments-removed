



#ifndef SANDBOX_SRC_NAMED_PIPE_POLICY_H__
#define SANDBOX_SRC_NAMED_PIPE_POLICY_H__

#include <string>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/sandbox_policy.h"

namespace sandbox {

enum EvalResult;


class NamedPipePolicy {
 public:
  
  
  
  
  
  static bool GenerateRules(const wchar_t* name,
                            TargetPolicy::Semantics semantics,
                            LowLevelPolicy* policy);

  
  static DWORD CreateNamedPipeAction(EvalResult eval_result,
                                     const ClientInfo& client_info,
                                     const base::string16 &name,
                                     DWORD open_mode, DWORD pipe_mode,
                                     DWORD max_instances,
                                     DWORD out_buffer_size,
                                     DWORD in_buffer_size,
                                     DWORD default_timeout, HANDLE* pipe);
};

}  


#endif  
