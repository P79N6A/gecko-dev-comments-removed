




































#ifndef NSEXTERNALURLHANDLERSERVICE_H
#define NSEXTERNALURLHANDLERSERVICE_H

#include "nsIExternalURLHandlerService.h"


#define NS_EXTERNALURLHANDLERSERVICE_CID \
    {0x4bf1f8ef, 0xd947, 0x4ba3, {0x9c, 0xd3, 0x8c, 0x9a, 0x54, 0xa6, 0x3a, 0x1c}}

class nsExternalURLHandlerService : public nsIExternalURLHandlerService
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIEXTERNALURLHANDLERSERVICE

    nsExternalURLHandlerService();
private:
    ~nsExternalURLHandlerService();
};

#endif 
