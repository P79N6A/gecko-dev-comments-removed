






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
        nsRefPtr<nsIdleServiceWin> idleService =
            nsIdleService::GetInstance().downcast<nsIdleServiceWin>();
        if (!idleService) {
            idleService = new nsIdleServiceWin();
        }
        
        return idleService.forget();
    }

protected:
    nsIdleServiceWin() { }
    virtual ~nsIdleServiceWin() { }
    bool UsePollMode();
};

#endif 
