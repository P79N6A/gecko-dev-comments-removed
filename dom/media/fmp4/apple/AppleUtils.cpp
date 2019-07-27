





#include "AppleUtils.h"
#include "prlog.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetAppleMediaLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("AppleMedia");
  }
  return log;
}
#define WARN(...) PR_LOG(GetAppleMediaLog(), PR_LOG_WARNING, (__VA_ARGS__))
#else
#define WARN(...)
#endif

#define PROPERTY_ID_FORMAT "%c%c%c%c"
#define PROPERTY_ID_PRINT(x) ((x) >> 24), \
                             ((x) >> 16) & 0xff, \
                             ((x) >> 8) & 0xff, \
                              (x) & 0xff
