





#ifndef AccessibleCaretLog_h
#define AccessibleCaretLog_h

#include "mozilla/Logging.h"

namespace mozilla {

PRLogModuleInfo* GetAccessibleCaretLog();

#ifndef AC_LOG_BASE
#define AC_LOG_BASE(...) MOZ_LOG(GetAccessibleCaretLog(), mozilla::LogLevel::Debug, (__VA_ARGS__));
#endif

#ifndef AC_LOGV_BASE
#define AC_LOGV_BASE(...)                                                      \
  MOZ_LOG(GetAccessibleCaretLog(), LogLevel::Verbose, (__VA_ARGS__));
#endif

} 

#endif 
