






































#ifndef mozilla_Hal_h
#define mozilla_Hal_h 1

#include "base/basictypes.h"
#include "mozilla/Types.h"
#include "nsTArray.h"

#ifndef MOZ_HAL_NAMESPACE











# include "HalImpl.h"
# include "HalSandbox.h"
# define MOZ_HAL_NAMESPACE hal
# define MOZ_DEFINED_HAL_NAMESPACE 1
#endif

namespace mozilla {
namespace MOZ_HAL_NAMESPACE  {









void Vibrate(const nsTArray<uint32>& pattern);

}
}

#ifdef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_DEFINED_HAL_NAMESPACE
# undef MOZ_HAL_NAMESPACE
#endif

#endif  
