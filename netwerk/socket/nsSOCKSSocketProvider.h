







































#ifndef nsSOCKSSocketProvider_h__
#define nsSOCKSSocketProvider_h__

#include "nsISocketProvider.h"


enum {
    NS_SOCKS_VERSION_4 = 4,
    NS_SOCKS_VERSION_5 = 5
};

class nsSOCKSSocketProvider : public nsISocketProvider
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISOCKETPROVIDER
    
    nsSOCKSSocketProvider(PRUint32 version) : mVersion(version) {}
    virtual ~nsSOCKSSocketProvider() {}
    
    static NS_METHOD CreateV4(nsISupports *, REFNSIID aIID, void **aResult);
    static NS_METHOD CreateV5(nsISupports *, REFNSIID aIID, void **aResult);
    
private:
    PRUint32 mVersion; 
};

#endif 
