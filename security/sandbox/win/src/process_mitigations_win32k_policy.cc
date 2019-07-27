



#include "sandbox/win/src/process_mitigations_win32k_policy.h"

namespace sandbox {

bool ProcessMitigationsWin32KLockdownPolicy::GenerateRules(
    const wchar_t* name,
    TargetPolicy::Semantics semantics,
    LowLevelPolicy* policy) {
  PolicyRule rule(FAKE_SUCCESS);
  if (!policy->AddRule(IPC_GDI_GDIDLLINITIALIZE_TAG, &rule))
    return false;
  if (!policy->AddRule(IPC_GDI_GETSTOCKOBJECT_TAG, &rule))
    return false;
  if (!policy->AddRule(IPC_USER_REGISTERCLASSW_TAG, &rule))
    return false;
  return true;
}

}  

