







#ifndef logging_h__
#define logging_h__

#include <sstream>

#include <prlog.h>

#if defined(PR_LOGGING)


#define MOZ_MTLOG_MODULE(n) \
  static PRLogModuleInfo* getLogModule() {      \
    static PRLogModuleInfo* log;                \
    if (!log)                                   \
      log = PR_NewLogModule(n);                 \
    return log;                                 \
  }

#define MOZ_MTLOG(level, b) \
  do {                                                                  \
    std::stringstream str;                                              \
    str << b;                                                           \
    PR_LOG(getLogModule(), level, ("%s", str.str().c_str())); } while(0)

#else


#define MOZ_MTLOG_MODULE(n)
#define MOZ_MTLOG(level, b)

#endif 

#endif 
