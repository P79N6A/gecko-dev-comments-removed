






#ifndef nsIdleServiceWin_h__
#define nsIdleServiceWin_h__

#include "nsIdleService.h"





#ifndef SAFE_COMPARE_EVEN_WITH_WRAPPING
#define SAFE_COMPARE_EVEN_WITH_WRAPPING(A, B) (((int)((long)A - (long)B) & 0xFFFFFFFF))
#endif


class nsIdleServiceWin : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS_INHERITED

    bool PollIdleTime(uint32_t* aIdleTime);

    static already_AddRefed<nsIdleServiceWin> GetInstance()
    {
        nsIdleServiceWin* idleService =
            static_cast<nsIdleServiceWin*>(nsIdleService::GetInstance().get());
        if (!idleService) {
            idleService = new nsIdleServiceWin();
            NS_ADDREF(idleService);
        }
        
        return idleService;
    }

protected:
    nsIdleServiceWin() { }
    virtual ~nsIdleServiceWin() { }
    bool UsePollMode();
};

#endif 
