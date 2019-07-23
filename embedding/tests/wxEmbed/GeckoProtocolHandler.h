





























#ifndef GECKOPROTOCOLHANDLER_H
#define GECKOPROTOCOLHANDLER_H

#include "nsIProtocolHandler.h"
#include "nsIChannel.h"
#include "nsIURI.h"

class GeckoChannelCallback
{
public:
    
    
    virtual nsresult GetData(
        nsIURI *aURI,
        nsIChannel *aChannel,
        nsACString &aContentType,
        void **aData,
        PRUint32 *aSize) = 0;
};

class GeckoProtocolHandler
{
public:
    static nsresult RegisterHandler(const char *aScheme, const char *aDescription, GeckoChannelCallback *aCallback);
};

#endif