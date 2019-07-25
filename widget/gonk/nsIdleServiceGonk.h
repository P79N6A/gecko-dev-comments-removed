
















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
        nsIdleServiceGonk* idleService =
            static_cast<nsIdleServiceGonk*>(nsIdleService::GetInstance().get());
        if (!idleService) {
            idleService = new nsIdleServiceGonk();
            NS_ADDREF(idleService);
        }

        return idleService;
    }

protected:
    nsIdleServiceGonk() { }
    virtual ~nsIdleServiceGonk() { }
    bool UsePollMode();
};

#endif 
