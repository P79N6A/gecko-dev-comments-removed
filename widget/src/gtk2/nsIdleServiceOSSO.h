



































#ifndef nsIdleServiceOSSO_h__
#define nsIdleServiceOSSO_h__

#include "nsIdleService.h"
#include "nsIObserver.h"

class nsIdleServiceOSSO : public nsIdleService, public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    nsIdleServiceOSSO();

    NS_IMETHOD GetIdleTime(PRUint32* idleTime);

private:
    ~nsIdleServiceOSSO();
    PRBool mIdle;
    PRTime mIdleSince;
};

#endif 
