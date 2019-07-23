





































#ifndef nsIdleServiceOS2_h__
#define nsIdleServiceOS2_h__

#include "nsIdleService.h"
#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#include <os2.h>

class nsIdleServiceOS2 : public nsIdleService
{
public:
  NS_DECL_ISUPPORTS

  nsIdleServiceOS2();
  ~nsIdleServiceOS2();

  
  NS_IMETHOD GetIdleTime(PRUint32 *aIdleTime);

private:
  HMODULE mHMod; 
  PRBool mInitialized; 
};

#endif 
