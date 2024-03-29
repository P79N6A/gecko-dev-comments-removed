







#ifndef logging_h__
#define logging_h__

#include <sstream>
#include "mozilla/Logging.h"

#define ML_ERROR            mozilla::LogLevel::Error
#define ML_WARNING          mozilla::LogLevel::Warning
#define ML_NOTICE           mozilla::LogLevel::Info
#define ML_INFO             mozilla::LogLevel::Debug
#define ML_DEBUG            mozilla::LogLevel::Verbose

#define MOZ_MTLOG_MODULE(n) \
  static PRLogModuleInfo* getLogModule() {      \
    static PRLogModuleInfo* log;                \
    if (!log)                                   \
      log = PR_NewLogModule(n);                 \
    return log;                                 \
  }

#define MOZ_MTLOG(level, b) \
  do {                                                                  \
    if (MOZ_LOG_TEST(getLogModule(), level)) {                           \
      std::stringstream str;                                            \
      str << b;                                                         \
      MOZ_LOG(getLogModule(), level, ("%s", str.str().c_str()));         \
    }                                                                   \
  } while(0)

#endif 
