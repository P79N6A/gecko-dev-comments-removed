







































#ifndef nsIdleServiceWin_h__
#define nsIdleServiceWin_h__

#include "nsIdleService.h"

class nsIdleServiceWin : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD GetIdleTime(PRUint32* idleTime);
};

#endif 
