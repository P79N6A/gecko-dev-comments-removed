



































#ifndef mozilla_dom_power_PowerManagerService_h
#define mozilla_dom_power_PowerManagerService_h

#include "nsIPowerManagerService.h"
#include "nsCOMPtr.h" 

namespace mozilla {
namespace dom {
namespace power {

class PowerManagerService
  : public nsIPowerManagerService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPOWERMANAGERSERVICE

  static already_AddRefed<nsIPowerManagerService> GetInstance();
};

} 
} 
} 

#endif 
