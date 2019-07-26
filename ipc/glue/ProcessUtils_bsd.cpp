



#include "ProcessUtils.h"

#include <pthread.h>

#if !defined(OS_NETBSD)
#include <pthread_np.h>
#endif

namespace mozilla {
namespace ipc {

void SetThisProcessName(const char *aName)
{
#if defined(OS_NETBSD)
  pthread_setname_np(pthread_self(), "%s", (void *)aName);
#else
  pthread_set_name_np(pthread_self(), aName);
#endif
}

} 
} 
