




































#ifndef nsDataHandler_h___
#define nsDataHandler_h___

#include "nsIProtocolHandler.h"

class nsDataHandler : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsDataHandler();
    virtual ~nsDataHandler();

    
    static NS_METHOD
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);
};

#endif 
