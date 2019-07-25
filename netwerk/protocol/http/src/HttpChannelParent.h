







































#ifndef mozilla_net_HttpChannelParent_h
#define mozilla_net_HttpChannelParent_h

#include "nsHttp.h"
#include "mozilla/net/PHttpChannelParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"

namespace mozilla {
namespace net {


class HttpChannelParent : public PHttpChannelParent
                        , public nsIStreamListener
                        , public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR

  HttpChannelParent();
  virtual ~HttpChannelParent();

protected:
  virtual bool RecvAsyncOpen(const nsCString&           uriSpec, 
                             const nsCString&           charset,
                             const nsCString&           originalUriSpec, 
                             const nsCString&           originalCharset,
                             const nsCString&           docUriSpec, 
                             const nsCString&           docCharset,
                             const PRUint32&            loadFlags,
                             const RequestHeaderTuples& requestHeaders);
};

} 
} 

#endif 
