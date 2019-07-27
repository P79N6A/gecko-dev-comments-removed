



#ifndef NSAUTODIALQT
#define NSAUTODIALQT

#include "nspr.h"
#include "nscore.h"

class nsAutodial
{
public:
    nsAutodial();
    ~nsAutodial();

    nsresult Init();

    
    nsresult DialDefault(const char16_t* hostName);

    
    bool ShouldDialOnNetworkError();
};

#endif 
