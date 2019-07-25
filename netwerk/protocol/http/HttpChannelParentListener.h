








































#ifndef mozilla_net_HttpChannelCallbackWrapper_h
#define mozilla_net_HttpChannelCallbackWrapper_h

#include "nsHttp.h"
#include "mozilla/net/NeckoCommon.h"
#include "PHttpChannelParams.h"
#include "nsIParentChannel.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "nsIRedirectResultListener.h"
#include "nsIProgressEventSink.h"

using namespace mozilla::dom;

class nsICacheEntryDescriptor;

namespace mozilla {
namespace net {

class HttpChannelParent;

class HttpChannelParentListener : public nsIInterfaceRequestor
                                 , public nsIChannelEventSink
                                 , public nsIRedirectResultListener
                                 , public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIREDIRECTRESULTLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  HttpChannelParentListener(HttpChannelParent* aInitialChannel);
  virtual ~HttpChannelParentListener();

private:
  nsCOMPtr<nsIParentChannel> mActiveChannel;
  PRUint32 mRedirectChannelId;
};

} 
} 

#endif 
