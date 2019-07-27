




#ifndef __POWERWAKELOCK_H_
#define __POWERWAKELOCK_H_

#include "mozilla/StaticPtr.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace hal_impl {
class PowerWakelock
{
public:
  NS_INLINE_DECL_REFCOUNTING(PowerWakelock);
  PowerWakelock();
private:
  ~PowerWakelock();
};
extern StaticRefPtr <PowerWakelock> gPowerWakelock;
} 
} 
#endif 
