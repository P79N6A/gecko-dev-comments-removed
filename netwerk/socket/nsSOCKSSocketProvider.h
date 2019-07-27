





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
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSISOCKETPROVIDER

    explicit nsSOCKSSocketProvider(uint32_t version) : mVersion(version) {}

    static nsresult CreateV4(nsISupports *, REFNSIID aIID, void **aResult);
    static nsresult CreateV5(nsISupports *, REFNSIID aIID, void **aResult);

private:
    virtual ~nsSOCKSSocketProvider() {}

    uint32_t mVersion; 
};

#endif 
