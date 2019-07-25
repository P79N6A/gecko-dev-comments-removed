








































#ifndef nsIdleServiceGonk_h__
#define nsIdleServiceGonk_h__

#include "nsIdleService.h"

class nsIdleServiceGonk : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS

    bool PollIdleTime(PRUint32* aIdleTime);
protected:
    bool UsePollMode();
};

#endif 
