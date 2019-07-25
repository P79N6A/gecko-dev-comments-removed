





#ifndef nsIdleServiceOS2_h__
#define nsIdleServiceOS2_h__

#include "nsIdleService.h"
#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#include <os2.h>

class nsIdleServiceOS2 : public nsIdleService
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  bool PollIdleTime(PRUint32 *aIdleTime);

  static already_AddRefed<nsIdleServiceOS2> GetInstance()
  {
    nsIdleServiceOS2* idleService =
      static_cast<nsIdleServiceOS2*>(nsIdleService::GetInstance().get());
    if (!idleService) {
      idleService = new nsIdleServiceOS2();
      NS_ADDREF(idleService);
    }
    
    return idleService;
  }
  
private:
  HMODULE mHMod; 
  bool mInitialized; 

protected:
  nsIdleServiceOS2();
  ~nsIdleServiceOS2();
  bool UsePollMode();
};

#endif 
