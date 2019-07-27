



#ifndef nsNativeConnectionHelper_h__
#define nsNativeConnectionHelper_h__

#include "nscore.h"

class nsNativeConnectionHelper
{
public:
    




    static bool OnConnectionFailed(const char16_t* hostName);

    



   
    static bool IsAutodialEnabled();
};

#endif 
