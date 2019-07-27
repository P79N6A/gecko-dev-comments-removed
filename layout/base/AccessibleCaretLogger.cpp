





#include "AccessibleCaretLogger.h"

namespace mozilla {

PRLogModuleInfo*
GetAccessibleCaretLog()
{
  static PRLogModuleInfo* log = nullptr;

  if (!log) {
    log = PR_NewLogModule("AccessibleCaret");
  }

  return log;
}

} 
