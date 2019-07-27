



#ifndef nsUDPSocketProvider_h__
#define nsUDPSocketProvider_h__

#include "nsISocketProvider.h"
#include "mozilla/Attributes.h"

class nsUDPSocketProvider final : public nsISocketProvider
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSISOCKETPROVIDER

private:
    ~nsUDPSocketProvider();

};

#endif 

