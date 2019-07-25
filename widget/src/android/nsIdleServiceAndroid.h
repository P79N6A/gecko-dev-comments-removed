







































#ifndef nsIdleServiceAndroid_h__
#define nsIdleServiceAndroid_h__

#include "nsIdleService.h"

class nsIdleServiceAndroid : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS

    bool PollIdleTime(PRUint32* aIdleTime);
protected:
    bool UsePollMode();
};

#endif 
