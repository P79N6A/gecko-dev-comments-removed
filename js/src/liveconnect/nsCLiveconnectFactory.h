












































#ifndef nsCLiveconnectFactory_h___
#define nsCLiveconnectFactory_h___

#include "nsISupports.h"
#include "nsIFactory.h"

class nsCLiveconnectFactory : public nsIFactory {
public:
    
    

    NS_DECL_ISUPPORTS

    
    

    NS_IMETHOD
    CreateInstance(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    NS_IMETHOD
    LockFactory(PRBool aLock);


    
    

    nsCLiveconnectFactory(void);
    virtual ~nsCLiveconnectFactory(void);
};

#endif 
