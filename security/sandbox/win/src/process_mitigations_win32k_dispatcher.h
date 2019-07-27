



#ifndef SANDBOX_SRC_PROCESS_MITIGATIONS_WIN32K_DISPATCHER_H_
#define SANDBOX_SRC_PROCESS_MITIGATIONS_WIN32K_DISPATCHER_H_

#include "base/basictypes.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {



class ProcessMitigationsWin32KDispatcher : public Dispatcher {
 public:
  explicit ProcessMitigationsWin32KDispatcher(PolicyBase* policy_base);
  ~ProcessMitigationsWin32KDispatcher() {}

  
  virtual bool SetupService(InterceptionManager* manager, int service);

 private:
  PolicyBase* policy_base_;

  DISALLOW_COPY_AND_ASSIGN(ProcessMitigationsWin32KDispatcher);
};

}  

#endif  
