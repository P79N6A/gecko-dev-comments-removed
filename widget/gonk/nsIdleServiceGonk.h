
















#ifndef nsIdleServiceGonk_h__
#define nsIdleServiceGonk_h__

#include "nsIdleService.h"

class nsIdleServiceGonk : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    bool PollIdleTime(uint32_t* aIdleTime);

    static already_AddRefed<nsIdleServiceGonk> GetInstance()
    {
        nsRefPtr<nsIdleServiceGonk> idleService =
            nsIdleService::GetInstance().downcast<nsIdleServiceGonk>();
        if (!idleService) {
            idleService = new nsIdleServiceGonk();
        }

        return idleService.forget();
    }

protected:
    nsIdleServiceGonk() { }
    virtual ~nsIdleServiceGonk() { }
    bool UsePollMode();
};

#endif 
