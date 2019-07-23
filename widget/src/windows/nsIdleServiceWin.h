







































#ifndef nsIdleServiceWin_h__
#define nsIdleServiceWin_h__

#include "nsIdleService.h"





#ifndef SAFE_COMPARE_EVEN_WITH_WRAPPING
#define SAFE_COMPARE_EVEN_WITH_WRAPPING(A, B) (((int)((long)A - (long)B) & 0xFFFFFFFF))
#endif


class nsIdleServiceWin : public nsIdleService
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD GetIdleTime(PRUint32* idleTime);
};

#endif 
