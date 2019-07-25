








































#ifndef mozilla_net_HttpChannelParent_h
#define mozilla_net_HttpChannelParent_h

#include "nsHttp.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/net/PHttpChannelParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIParentRedirectingChannel.h"
#include "nsIProgressEventSink.h"
#include "nsITabParent.h"

using namespace mozilla::dom;

class nsICacheEntryDescriptor;

namespace mozilla {
namespace net {

class HttpChannelParentListener;

class HttpChannelParent : public PHttpChannelParent
                        , public nsIParentRedirectingChannel
                        , public nsIProgressEventSink
                        , public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIPARENTCHANNEL
  NS_DECL_NSIPARENTREDIRECTINGCHANNEL
  NS_DECL_NSIPROGRESSEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR

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
                             const IPC::InputStream&    uploadStream,
                             const PRBool&              uploadStreamHasHeaders,
                             const PRUint16&            priority,
                             const PRUint8&             redirectionLimit,
                             const PRBool&              allowPipelining,
                             const PRBool&              forceAllowThirdPartyCookie,
                             const bool&                doResumeAt,
                             const PRUint64&            startPos,
                             const nsCString&           entityID,
                             const bool&                chooseApplicationCache,
                             const nsCString&           appCacheClientID);

  virtual bool RecvConnectChannel(const PRUint32& channelId);
  virtual bool RecvSetPriority(const PRUint16& priority);
  virtual bool RecvSetCacheTokenCachedCharset(const nsCString& charset);
  virtual bool RecvSuspend();
  virtual bool RecvResume();
  virtual bool RecvCancel(const nsresult& status);
  virtual bool RecvRedirect2Verify(const nsresult& result,
                                   const RequestHeaderTuples& changedHeaders);
  virtual bool RecvUpdateAssociatedContentSecurity(const PRInt32& high,
                                                   const PRInt32& low,
                                                   const PRInt32& broken,
                                                   const PRInt32& no);
  virtual bool RecvDocumentChannelCleanup();
  virtual bool RecvMarkOfflineCacheEntryAsForeign();

  virtual void ActorDestroy(ActorDestroyReason why);

protected:
  friend class mozilla::net::HttpChannelParentListener;
  nsCOMPtr<nsITabParent> mTabParent;

private:
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsICacheEntryDescriptor> mCacheDescriptor;
  bool mIPCClosed;                

  nsCOMPtr<nsIChannel> mRedirectChannel;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;

  
  
  nsresult mStoredStatus;
  PRUint64 mStoredProgress;
  PRUint64 mStoredProgressMax;
};

} 
} 

#endif 
