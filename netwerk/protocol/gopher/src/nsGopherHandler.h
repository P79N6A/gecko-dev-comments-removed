






































#ifndef nsGopherHandler_h___
#define nsGopherHandler_h___

#include "nsIProxiedProtocolHandler.h"
#include "nsIProtocolProxyService.h"
#include "nsString.h"
#include "nsCOMPtr.h"

#define GOPHER_PORT 70



#define NS_GOPHERHANDLER_CID     \
{ 0x44588c1f, 0x2ce8, 0x4ad8, \
   {0x9b, 0x16, 0xdf, 0xb9, 0xd9, 0xd5, 0x13, 0xa7} }

class nsGopherHandler : public nsIProxiedProtocolHandler {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIPROXIEDPROTOCOLHANDLER

    
    nsGopherHandler() {}

protected:
    nsCOMPtr<nsIProtocolProxyService> mProxySvc;
};

#endif 
