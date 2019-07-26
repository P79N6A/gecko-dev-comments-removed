



#ifndef SANDBOX_SRC_REGISTRY_POLICY_H__
#define SANDBOX_SRC_REGISTRY_POLICY_H__

#include <string>

#include "base/basictypes.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/sandbox_policy.h"

namespace sandbox {

enum EvalResult;


class RegistryPolicy {
 public:
  
  
  static bool GenerateRules(const wchar_t* name,
                            TargetPolicy::Semantics semantics,
                            LowLevelPolicy* policy);

  
  
  static bool CreateKeyAction(EvalResult eval_result,
                              const ClientInfo& client_info,
                              const std::wstring &key,
                              uint32 attributes,
                              HANDLE root_directory,
                              uint32 desired_access,
                              uint32 title_index,
                              uint32 create_options,
                              HANDLE* handle,
                              NTSTATUS* nt_status,
                              ULONG* disposition);

  
  
  static bool OpenKeyAction(EvalResult eval_result,
                              const ClientInfo& client_info,
                              const std::wstring &key,
                              uint32 attributes,
                              HANDLE root_directory,
                              uint32 desired_access,
                              HANDLE* handle,
                              NTSTATUS* nt_status);
};

}  

#endif  
