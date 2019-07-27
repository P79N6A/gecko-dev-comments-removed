



#ifndef SANDBOX_SRC_HANDLE_CLOSER_AGENT_H_
#define SANDBOX_SRC_HANDLE_CLOSER_AGENT_H_

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/handle_closer.h"
#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {


class HandleCloserAgent {
 public:
  HandleCloserAgent() {}

  
  void InitializeHandlesToClose();

  
  bool CloseHandles();

  
  static bool NeedsHandlesClosed();

 private:
  HandleMap handles_to_close_;

  DISALLOW_COPY_AND_ASSIGN(HandleCloserAgent);
};

}  

#endif  
