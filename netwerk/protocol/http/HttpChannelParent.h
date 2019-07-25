








































#ifndef mozilla_net_HttpChannelParent_h
#define mozilla_net_HttpChannelParent_h

#include "nsHttp.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/net/PHttpChannelParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIProgressEventSink.h"
#include "nsITabParent.h"

using namespace mozilla::dom;

class nsICacheEntryDescriptor;

namespace mozilla {
namespace net {

class HttpChannelParentListener;

class HttpChannelParent : public PHttpChannelParent
                        , public nsIProgressEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROGRESSEVENTSINK

  
  nsresult OnStartRequest(nsIRequest *aRequest, 
                          nsISupports *aContext);
  nsresult OnStopRequest(nsIRequest *aRequest, 
                         nsISupports *aContext, 
                         nsresult aStatusCode);
  nsresult OnDataAvailable(nsIRequest *aRequest, 
                           nsISupports *aContext, 
                           nsIInputStream *aInputStream, 
                           PRUint32 aOffset, 
                           PRUint32 aCount);
  
  HttpChannelParent(PBrowserParent* iframeEmbedding);
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
                             const PRBool&              forceAllowThirdPartyCookie,
                             const bool&                doResumeAt,
                             const PRUint64&            startPos,
                             const nsCString&           entityID);

  virtual bool RecvSetPriority(const PRUint16& priority);
  virtual bool RecvSetCacheTokenCachedCharset(const nsCString& charset);
  virtual bool RecvSuspend();
  virtual bool RecvResume();
  virtual bool RecvCancel(const nsresult& status);
  virtual bool RecvRedirect2Result(const nsresult& result,
                                   const RequestHeaderTuples& changedHeaders);
  virtual bool RecvUpdateAssociatedContentSecurity(const PRInt32& high,
                                                   const PRInt32& low,
                                                   const PRInt32& broken,
                                                   const PRInt32& no);

  virtual void ActorDestroy(ActorDestroyReason why);

protected:
  friend class mozilla::net::HttpChannelParentListener;
  nsCOMPtr<nsITabParent> mTabParent;

private:
  nsCOMPtr<nsIChannel> mChannel;
  nsRefPtr<HttpChannelParentListener> mChannelListener;
  nsCOMPtr<nsICacheEntryDescriptor> mCacheDescriptor;
  bool mIPCClosed;                
};

} 
} 

#endif 
