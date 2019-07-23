




































#ifndef nsJSProtocolHandler_h___
#define nsJSProtocolHandler_h___

#include "nsIProtocolHandler.h"
#include "nsITextToSubURI.h"

#define NS_JSPROTOCOLHANDLER_CID                     \
{ /* bfc310d2-38a0-11d3-8cd3-0060b0fc14a3 */         \
    0xbfc310d2,                                      \
    0x38a0,                                          \
    0x11d3,                                          \
    {0x8c, 0xd3, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

#define NS_JSPROTOCOLHANDLER_CONTRACTID \
    NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "javascript"


class nsJSProtocolHandler : public nsIProtocolHandler
{
public:
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsJSProtocolHandler();
    virtual ~nsJSProtocolHandler();

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    nsresult Init();

protected:

    nsresult EnsureUTF8Spec(const nsAFlatCString &aSpec, const char *aCharset, 
                            nsACString &aUTF8Spec);

    nsCOMPtr<nsITextToSubURI>  mTextToSubURI;
};

#endif 
