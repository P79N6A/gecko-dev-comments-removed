







#ifndef logging_h__
#define logging_h__

#include <sstream>
#include "mozilla/Logging.h"

#define ML_EMERG            1
#define ML_ERROR            2
#define ML_WARNING          3
#define ML_NOTICE           4
#define ML_INFO             5
#define ML_DEBUG            6

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
