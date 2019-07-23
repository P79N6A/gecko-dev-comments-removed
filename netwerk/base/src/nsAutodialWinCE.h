




































#ifndef nsAutodialWin_h__
#define nsAutodialWin_h__

#include "nspr.h"
#include "nscore.h"

#include <windows.h>


class nsRASAutodial
{
public:
  
    
    nsRASAutodial();

    
    virtual ~nsRASAutodial();

    
    
    nsresult Init();

    
    nsresult DialDefault(const PRUnichar* hostName);

    
    PRBool ShouldDialOnNetworkError();
};


#endif
