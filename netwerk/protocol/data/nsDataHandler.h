




































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

    
    static nsresult
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);

    
    
    
    static NS_HIDDEN_(nsresult) ParseURI(nsCString& spec,
                                         nsCString& contentType,
                                         nsCString& contentCharset,
                                         PRBool&    isBase64,
                                         nsCString& dataBuffer,
                                         nsCString& hashRef);
};

#endif 
