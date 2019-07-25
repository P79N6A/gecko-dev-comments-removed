






































#include "Hal.h"
#include "mozilla/Util.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#define PROXY_IF_SANDBOXED(_call)                 \
  do {                                            \
    if (InSandbox()) {                            \
      hal_sandbox::_call;                         \
    } else {                                      \
      hal_impl::_call;                            \
    }                                             \
  } while (0)

namespace mozilla {
namespace hal {

static void
AssertMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
}

static bool
InSandbox()
{
  return GeckoProcessType_Content == XRE_GetProcessType();
}

void
Vibrate(const nsTArray<uint32>& pattern)
{
  AssertMainThread();
  PROXY_IF_SANDBOXED(Vibrate(pattern));
}

} 
} 
