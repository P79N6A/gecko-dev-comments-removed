



#ifndef SANDBOX_SRC_SYNC_POLICY_H__
#define SANDBOX_SRC_SYNC_POLICY_H__

#include <string>

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/sandbox_policy.h"

namespace sandbox {

enum EvalResult;


class SyncPolicy {
 public:
  
  
  
  
  
  static bool GenerateRules(const wchar_t* name,
                            TargetPolicy::Semantics semantics,
                            LowLevelPolicy* policy);

  
  
  
  static NTSTATUS CreateEventAction(EvalResult eval_result,
                                    const ClientInfo& client_info,
                                    const base::string16 &event_name,
                                    uint32 event_type,
                                    uint32 initial_state,
                                    HANDLE *handle);
  static NTSTATUS OpenEventAction(EvalResult eval_result,
                                  const ClientInfo& client_info,
                                  const base::string16 &event_name,
                                  uint32 desired_access,
                                  HANDLE *handle);
};

}  

#endif  
