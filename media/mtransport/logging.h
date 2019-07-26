







#ifndef logging_h__
#define logging_h__

#if defined(PR_LOG)
#error "Must #include logging.h before before any IPDL-generated files or other files that #include prlog.h."
#endif


#ifndef PR_LOGGING
#define FORCE_PR_LOG 1
#endif

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
