







































#ifndef mozilla_net_NeckoCommon_h
#define mozilla_net_NeckoCommon_h

#include "nsXULAppAPI.h"






#define DROP_DEAD()                                                            \
  do {                                                                         \
    fprintf(stderr,                                                            \
            "*&*&*&*&*&*&*&**&*&&*& FATAL ERROR: '%s' UNIMPLEMENTED: %s +%d",  \
            __FUNCTION__, __FILE__, __LINE__);                                 \
    NS_ABORT();                                                                \
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
    const char * e = PR_GetEnv("NECKO_E10S_HTTP");
    if (e && *e)
      amChild = (XRE_GetProcessType() == GeckoProcessType_Content);
    didCheck = true;
  }
  return amChild;
}


} 
} 

#endif 

