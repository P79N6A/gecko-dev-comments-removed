




#ifndef nsChromeProtocolHandler_h___
#define nsChromeProtocolHandler_h___

#include "nsIProtocolHandler.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"

#define NS_CHROMEPROTOCOLHANDLER_CID                 \
{ /* 61ba33c0-3031-11d3-8cd0-0060b0fc14a3 */         \
    0x61ba33c0,                                      \
    0x3031,                                          \
    0x11d3,                                          \
    {0x8c, 0xd0, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

class nsChromeProtocolHandler MOZ_FINAL : public nsIProtocolHandler,
                                          public nsSupportsWeakReference
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS

    
    NS_DECL_NSIPROTOCOLHANDLER

    
    nsChromeProtocolHandler() {}
    ~nsChromeProtocolHandler() {}
};

#endif 
