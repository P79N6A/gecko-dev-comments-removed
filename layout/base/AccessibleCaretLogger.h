





#ifndef AccessibleCaretLog_h
#define AccessibleCaretLog_h

#include "mozilla/Logging.h"

namespace mozilla {

PRLogModuleInfo* GetAccessibleCaretLog();

#ifndef AC_LOG_BASE
#define AC_LOG_BASE(...) PR_LOG(GetAccessibleCaretLog(), PR_LOG_DEBUG, (__VA_ARGS__));
#endif

#ifndef AC_LOGV_BASE
#define AC_LOGV_BASE(...)                                                      \
  PR_LOG(GetAccessibleCaretLog(), PR_LOG_DEBUG + 1, (__VA_ARGS__));
#endif

} 

#endif 
