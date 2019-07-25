







































#ifndef mozilla_net_NeckoCommon_h
#define mozilla_net_NeckoCommon_h

#include "nsXULAppAPI.h"
#include "prenv.h"

#if defined(DEBUG) || defined(ENABLE_TESTS)
# define NECKO_ERRORS_ARE_FATAL_DEFAULT true
#else
# define NECKO_ERRORS_ARE_FATAL_DEFAULT false
#endif 





#define NECKO_MAYBE_ABORT(msg)                                                 \
  do {                                                                         \
    bool abort = NECKO_ERRORS_ARE_FATAL_DEFAULT;                               \
    const char *e = PR_GetEnv("NECKO_ERRORS_ARE_FATAL");                       \
    if (e)                                                                     \
      abort = (*e == '0') ? false : true;                                      \
    if (abort) {                                                               \
      msg.Append(" (set NECKO_ERRORS_ARE_FATAL=0 in your environment to "      \
                      "convert this error into a warning.)");                  \
      NS_RUNTIMEABORT(msg.get());                                              \
    } else {                                                                   \
      msg.Append(" (set NECKO_ERRORS_ARE_FATAL=1 in your environment to "      \
                      "convert this warning into a fatal error.)");            \
      NS_WARNING(msg.get());                                                   \
    }                                                                          \
  } while (0)

#define DROP_DEAD()                                                            \
  do {                                                                         \
    nsPrintfCString msg(1000,"FATAL NECKO ERROR: '%s' UNIMPLEMENTED",          \
                        __FUNCTION__);                                         \
    NECKO_MAYBE_ABORT(msg);                                                    \
    return NS_ERROR_NOT_IMPLEMENTED;                                           \
  } while (0)


namespace mozilla {
namespace net {

inline bool 
IsNeckoChild() 
{
  static bool didCheck = false;
  static bool amChild = false;

  if (!didCheck) {
    
    
    
    const char * e = PR_GetEnv("NECKO_SEPARATE_STACKS");
    if (!e) 
      amChild = (XRE_GetProcessType() == GeckoProcessType_Content);
    didCheck = true;
  }
  return amChild;
}


} 
} 

#endif 

