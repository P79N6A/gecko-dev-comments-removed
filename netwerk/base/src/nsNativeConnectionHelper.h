




































#ifndef nsNativeConnectionHelper_h__
#define nsNativeConnectionHelper_h__

#include "nscore.h"

class nsISocketTransport;

class nsNativeConnectionHelper
{
public:
    




    static PRBool OnConnectionFailed(const PRUnichar* hostName);

    



   
    static PRBool IsAutodialEnabled();
};

#endif 
