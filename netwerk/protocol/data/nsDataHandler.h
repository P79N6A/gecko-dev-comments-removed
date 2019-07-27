




#ifndef nsDataHandler_h___
#define nsDataHandler_h___

#include "nsIProtocolHandler.h"
#include "nsWeakReference.h"

class nsDataHandler : public nsIProtocolHandler
                    , public nsSupportsWeakReference
{
    virtual ~nsDataHandler();

public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsDataHandler();

    
    static nsresult
    Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult);

    
    
    
    static nsresult ParseURI(nsCString& spec,
                                         nsCString& contentType,
                                         nsCString& contentCharset,
                                         bool&    isBase64,
                                         nsCString& dataBuffer,
                                         nsCString& hashRef);
};

#endif 
