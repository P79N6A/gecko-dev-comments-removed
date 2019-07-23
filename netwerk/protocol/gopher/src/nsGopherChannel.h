







































#ifndef nsGopherChannel_h__
#define nsGopherChannel_h__

#include "nsBaseChannel.h"
#include "nsIProxyInfo.h"
#include "nsIProxiedChannel.h"

class nsGopherChannel : public nsBaseChannel, public nsIProxiedChannel {
public:
    nsGopherChannel(nsIURI *uri, nsIProxyInfo *pi) : mProxyInfo(pi) {
        SetURI(uri);
    }

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIPROXIEDCHANNEL

    nsIProxyInfo *ProxyInfo() { return mProxyInfo; }

protected:
    virtual ~nsGopherChannel() {}

    virtual nsresult OpenContentStream(PRBool async, nsIInputStream **result);
    virtual PRBool GetStatusArg(nsresult status, nsString &statusArg);

private:
    nsresult SendRequest(nsIOutputStream *stream);

    nsCOMPtr<nsIProxyInfo> mProxyInfo;
};

#endif 
