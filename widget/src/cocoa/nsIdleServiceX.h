



































#ifndef nsIdleServiceX_h_
#define nsIdleServiceX_h_

#include "nsIdleService.h"

class nsIdleServiceX : public nsIdleService
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetIdleTime(PRUint32* idleTime);
};

#endif 
