






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






class FTPChannelChild final : public PFTPChannelChild
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

  NS_IMETHOD Cancel(nsresult status) override;
  NS_IMETHOD Suspend() override;
  NS_IMETHOD Resume() override;

  explicit FTPChannelChild(nsIURI* uri);

  void AddIPDLReference();
  void ReleaseIPDLReference();

  NS_IMETHOD AsyncOpen(nsIStreamListener* listener, nsISupports* aContext) override;

  
  
  NS_IMETHOD IsPending(bool* result) override;

  nsresult OpenContentStream(bool async,
                             nsIInputStream** stream,
                             nsIChannel** channel) override;

  bool IsSuspended();

  void FlushedForDiversion();

protected:
  virtual ~FTPChannelChild();

  bool RecvOnStartRequest(const nsresult& aChannelStatus,
                          const int64_t& aContentLength,
                          const nsCString& aContentType,
                          const PRTime& aLastModified,
                          const nsCString& aEntityID,
                          const URIParams& aURI) override;
  bool RecvOnDataAvailable(const nsresult& channelStatus,
                           const nsCString& data,
                           const uint64_t& offset,
                           const uint32_t& count) override;
  bool RecvOnStopRequest(const nsresult& channelStatus) override;
  bool RecvFailedAsyncOpen(const nsresult& statusCode) override;
  bool RecvFlushedForDiversion() override;
  bool RecvDivertMessages() override;
  bool RecvDeleteSelf() override;

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
