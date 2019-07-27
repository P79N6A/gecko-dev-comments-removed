






#ifndef mozilla_net_FTPChannelChild_h
#define mozilla_net_FTPChannelChild_h

#include "mozilla/net/PFTPChannelChild.h"
#include "mozilla/net/ChannelEventQueue.h"
#include "nsBaseChannel.h"
#include "nsIFTPChannel.h"
#include "nsIUploadChannel.h"
#include "nsIProxiedChannel.h"
#include "nsIResumableChannel.h"
#include "nsIChildChannel.h"
#include "nsIDivertableChannel.h"

#include "nsIStreamListener.h"
#include "PrivateBrowsingChannel.h"

namespace mozilla {
namespace net {






class FTPChannelChild : public PFTPChannelChild
                      , public nsBaseChannel
                      , public nsIFTPChannel
                      , public nsIUploadChannel
                      , public nsIResumableChannel
                      , public nsIProxiedChannel
                      , public nsIChildChannel
                      , public nsIDivertableChannel
{
public:
  typedef ::nsIStreamListener nsIStreamListener;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIFTPCHANNEL
  NS_DECL_NSIUPLOADCHANNEL
  NS_DECL_NSIRESUMABLECHANNEL
  NS_DECL_NSIPROXIEDCHANNEL
  NS_DECL_NSICHILDCHANNEL
  NS_DECL_NSIDIVERTABLECHANNEL

  NS_IMETHOD Cancel(nsresult status);
  NS_IMETHOD Suspend();
  NS_IMETHOD Resume();

  explicit FTPChannelChild(nsIURI* uri);

  void AddIPDLReference();
  void ReleaseIPDLReference();

  NS_IMETHOD AsyncOpen(nsIStreamListener* listener, nsISupports* aContext);

  
  
  NS_IMETHOD IsPending(bool* result);

  nsresult OpenContentStream(bool async,
                             nsIInputStream** stream,
                             nsIChannel** channel) MOZ_OVERRIDE;

  bool IsSuspended();

  void FlushedForDiversion();

protected:
  virtual ~FTPChannelChild();

  bool RecvOnStartRequest(const nsresult& aChannelStatus,
                          const int64_t& aContentLength,
                          const nsCString& aContentType,
                          const PRTime& aLastModified,
                          const nsCString& aEntityID,
                          const URIParams& aURI) MOZ_OVERRIDE;
  bool RecvOnDataAvailable(const nsresult& channelStatus,
                           const nsCString& data,
                           const uint64_t& offset,
                           const uint32_t& count) MOZ_OVERRIDE;
  bool RecvOnStopRequest(const nsresult& channelStatus) MOZ_OVERRIDE;
  bool RecvFailedAsyncOpen(const nsresult& statusCode) MOZ_OVERRIDE;
  bool RecvFlushedForDiversion() MOZ_OVERRIDE;
  bool RecvDivertMessages() MOZ_OVERRIDE;
  bool RecvDeleteSelf() MOZ_OVERRIDE;

  void DoOnStartRequest(const nsresult& aChannelStatus,
                        const int64_t& aContentLength,
                        const nsCString& aContentType,
                        const PRTime& aLastModified,
                        const nsCString& aEntityID,
                        const URIParams& aURI);
  void DoOnDataAvailable(const nsresult& channelStatus,
                         const nsCString& data,
                         const uint64_t& offset,
                         const uint32_t& count);
  void DoOnStopRequest(const nsresult& statusCode);
  void DoFailedAsyncOpen(const nsresult& statusCode);
  void DoDeleteSelf();

  friend class FTPStartRequestEvent;
  friend class FTPDataAvailableEvent;
  friend class FTPStopRequestEvent;
  friend class FTPFailedAsyncOpenEvent;
  friend class FTPDeleteSelfEvent;

private:
  nsCOMPtr<nsIInputStream> mUploadStream;

  bool mIPCOpen;
  nsRefPtr<ChannelEventQueue> mEventQ;
  bool mCanceled;
  uint32_t mSuspendCount;
  bool mIsPending;
  bool mWasOpened;
  
  PRTime mLastModifiedTime;
  uint64_t mStartPos;
  nsCString mEntityID;

  
  bool mDivertingToParent;
  
  
  bool mFlushedForDiversion;
  
  
  bool mSuspendSent;
};

inline bool
FTPChannelChild::IsSuspended()
{
  return mSuspendCount != 0;
}

} 
} 

#endif 
