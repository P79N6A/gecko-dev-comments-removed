





#ifndef mozilla_logging_h
#define mozilla_logging_h

#include "prlog.h"






#define PR_LOG_INFO PR_LOG_DEBUG
#define PR_LOG_VERBOSE (PR_LOG_DEBUG + 1)

#define MOZ_LOG PR_LOG



#define MOZ_LOG_TEST(_module, _level) \
  ((_module) && (_module)->level >= (_level))

#endif 

