




































#ifndef nsAutodialMaemo_h__
#define nsAutodialMaemo_h__

#include "nspr.h"
#include "nscore.h"

class nsAutodial
{
public:
    nsAutodial();
    ~nsAutodial();

    nsresult Init();

    
    nsresult DialDefault(const PRUnichar* hostName);

    
    PRBool ShouldDialOnNetworkError();
};

#endif 
