



#ifndef mozilla_net_WyciwygChannelChild_h
#define mozilla_net_WyciwygChannelChild_h

#include "mozilla/net/PWyciwygChannelChild.h"
#include "mozilla/net/ChannelEventQueue.h"
#include "nsIWyciwygChannel.h"
#include "nsIChannel.h"
#include "nsIProgressEventSink.h"
#include "PrivateBrowsingChannel.h"

namespace mozilla {
namespace net {


enum WyciwygChannelChildState {
  WCC_NEW,
  WCC_INIT,

  
  WCC_OPENED,
  WCC_ONSTART,
  WCC_ONDATA,
  WCC_ONSTOP,

  
  WCC_ONWRITE,
  WCC_ONCLOSED
};



class WyciwygChannelChild : public PWyciwygChannelChild
                          , public nsIWyciwygChannel
                          , public PrivateBrowsingChannel<WyciwygChannelChild>
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIWYCIWYGCHANNEL

  WyciwygChannelChild();
  virtual ~WyciwygChannelChild();

  void AddIPDLReference();
  void ReleaseIPDLReference();

  nsresult Init(nsIURI *uri);

  bool IsSuspended();

protected:
  bool RecvOnStartRequest(const nsresult& statusCode,
                          const int64_t& contentLength,
                          const int32_t& source,
                          const nsCString& charset,
                          const nsCString& securityInfo);
  bool RecvOnDataAvailable(const nsCString& data,
                           const uint64_t& offset);
  bool RecvOnStopRequest(const nsresult& statusCode);
  bool RecvCancelEarly(const nsresult& statusCode);

  void OnStartRequest(const nsresult& statusCode,
                      const int64_t& contentLength,
                      const int32_t& source,
                      const nsCString& charset,
                      const nsCString& securityInfo);
  void OnDataAvailable(const nsCString& data,
                       const uint64_t& offset);
  void OnStopRequest(const nsresult& statusCode);
  void CancelEarly(const nsresult& statusCode);

  friend class PrivateBrowsingChannel<WyciwygChannelChild>;

private:
  nsresult                          mStatus;
  bool                              mIsPending;
  bool                              mCanceled;
  uint32_t                          mLoadFlags;
  int64_t                           mContentLength;
  int32_t                           mCharsetSource;
  nsCString                         mCharset;
  nsCOMPtr<nsIURI>                  mURI;
  nsCOMPtr<nsIURI>                  mOriginalURI;
  nsCOMPtr<nsISupports>             mOwner;
  nsCOMPtr<nsIInterfaceRequestor>   mCallbacks;
  nsCOMPtr<nsIProgressEventSink>    mProgressSink;
  nsCOMPtr<nsILoadGroup>            mLoadGroup;
  nsCOMPtr<nsIStreamListener>       mListener;
  nsCOMPtr<nsISupports>             mListenerContext;
  nsCOMPtr<nsISupports>             mSecurityInfo;

  
  enum WyciwygChannelChildState mState;

  bool mIPCOpen;
  bool mSentAppData;
  ChannelEventQueue mEventQ;

  friend class WyciwygStartRequestEvent;
  friend class WyciwygDataAvailableEvent;
  friend class WyciwygStopRequestEvent;
  friend class WyciwygCancelEvent;
};

inline bool
WyciwygChannelChild::IsSuspended()
{
  return false;
}

} 
} 

#endif 
