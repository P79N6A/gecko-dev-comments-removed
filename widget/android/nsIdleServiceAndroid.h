






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
        nsIdleServiceAndroid* idleService = 
            static_cast<nsIdleServiceAndroid*>(nsIdleService::GetInstance().get());
        if (!idleService) {
            idleService = new nsIdleServiceAndroid();
            NS_ADDREF(idleService);
        }
        
        return idleService;
    }

protected:
    nsIdleServiceAndroid() { }
    virtual ~nsIdleServiceAndroid() { }
    bool UsePollMode();
};

#endif 
