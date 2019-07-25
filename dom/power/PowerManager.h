



































#ifndef mozilla_dom_power_PowerManager_h
#define mozilla_dom_power_PowerManager_h

#include "nsIDOMPowerManager.h"

namespace mozilla {
namespace dom {
namespace power {

class PowerManager
  : public nsIDOMMozPowerManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZPOWERMANAGER

  PowerManager() {};
  virtual ~PowerManager() {};
};

} 
} 
} 

#endif 
