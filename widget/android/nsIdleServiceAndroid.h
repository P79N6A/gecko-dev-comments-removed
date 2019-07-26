






#ifndef nsIdleServiceAndroid_h__
#define nsIdleServiceAndroid_h__

#include "nsIdleService.h"

class nsIdleServiceAndroid : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    bool PollIdleTime(uint32_t* aIdleTime);

    static already_AddRefed<nsIdleServiceAndroid> GetInstance() 
    {
        nsRefPtr<nsIdleService> idleService = nsIdleService::GetInstance();
        if (!idleService) {
            idleService = new nsIdleServiceAndroid();
        }
        
        return idleService.forget().downcast<nsIdleServiceAndroid>();
    }

protected:
    nsIdleServiceAndroid() { }
    virtual ~nsIdleServiceAndroid() { }
    bool UsePollMode();
};

#endif 
