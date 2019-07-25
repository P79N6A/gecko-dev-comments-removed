







































#ifndef mozilla_net_HttpChannelParent_h
#define mozilla_net_HttpChannelParent_h

#include "nsHttp.h"
#include "mozilla/dom/PIFrameEmbeddingParent.h"
#include "mozilla/net/PHttpChannelParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"
#include "nsITabParent.h"

using namespace mozilla::dom;

namespace mozilla {
namespace net {


class HttpChannelParent : public PHttpChannelParent
                        , public nsIStreamListener
                        , public nsIInterfaceRequestor
                        , public nsIProgressEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIPROGRESSEVENTSINK

  HttpChannelParent(PIFrameEmbeddingParent* iframeEmbedding);
  virtual ~HttpChannelParent();

protected:
  virtual bool RecvAsyncOpen(const IPC::URI&            uri,
                             const IPC::URI&            originalUri,
                             const IPC::URI&            docUri,
                             const IPC::URI&            referrerUri,
                             const PRUint32&            loadFlags,
                             const RequestHeaderTuples& requestHeaders,
                             const nsHttpAtom&          requestMethod,
                             const nsCString&           uploadStreamData,
                             const PRInt32&             uploadStreamInfo,
                             const PRUint16&            priority,
                             const PRUint8&             redirectionLimit,
                             const PRBool&              allowPipelining,
                             const PRBool&              forceAllowThirdPartyCookie);

  virtual bool RecvSetPriority(const PRUint16& priority);

  virtual void ActorDestroy(ActorDestroyReason why);

  nsCOMPtr<nsITabParent> mTabParent;
  nsCOMPtr<nsIChannel> mChannel;
  bool mIPCClosed;                
};

} 
} 

#endif 
