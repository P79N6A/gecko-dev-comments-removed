



































#ifndef OJITestLoaderFactory_h___
#define OJITestLoaderFactory_h___

#include "nsISupports.h"
#include "nsIFactory.h"

class OJITestLoaderFactory : public nsIFactory {
public:

    NS_DECL_ISUPPORTS

    NS_IMETHOD
    CreateInstance(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    NS_IMETHOD
    LockFactory(PRBool aLock);

    OJITestLoaderFactory();
    virtual ~OJITestLoaderFactory();
};

#endif 
