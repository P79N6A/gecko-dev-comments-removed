



































#ifndef nsUDPSocketProvider_h__
#define nsUDPSocketProvider_h__

#include "nsISocketProvider.h"

class nsUDPSocketProvider : public nsISocketProvider
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISOCKETPROVIDER

private:
    ~nsUDPSocketProvider();

};

#endif 

