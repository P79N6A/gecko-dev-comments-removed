





#include "AccessibleCaretLogger.h"

namespace mozilla {

#ifdef PR_LOGGING

PRLogModuleInfo*
GetAccessibleCaretLog()
{
  static PRLogModuleInfo* log = nullptr;

  if (!log) {
    log = PR_NewLogModule("AccessibleCaret");
  }

  return log;
}

#endif 

} 
