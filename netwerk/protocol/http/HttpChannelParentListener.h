






#ifndef mozilla_net_HttpChannelCallbackWrapper_h
#define mozilla_net_HttpChannelCallbackWrapper_h

#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "nsIRedirectResultListener.h"

namespace mozilla {
namespace net {

class HttpChannelParent;

class HttpChannelParentListener final : public nsIInterfaceRequestor
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

  explicit HttpChannelParentListener(HttpChannelParent* aInitialChannel);

  
  nsresult DivertTo(nsIStreamListener *aListener);
  nsresult SuspendForDiversion();

private:
  virtual ~HttpChannelParentListener();

  
  nsresult ResumeForDiversion();

  
  
  
  
  nsCOMPtr<nsIStreamListener> mNextListener;
  uint32_t mRedirectChannelId;
  
  bool mSuspendedForDiversion;
};

} 
} 

#endif 
