








































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

#include "nsIStreamListener.h"

namespace mozilla {
namespace net {






class FTPChannelChild : public PFTPChannelChild
                      , public nsBaseChannel
                      , public nsIFTPChannel
                      , public nsIUploadChannel
                      , public nsIResumableChannel
                      , public nsIProxiedChannel
                      , public nsIChildChannel
{
public:
  typedef ::nsIStreamListener nsIStreamListener;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIFTPCHANNEL
  NS_DECL_NSIUPLOADCHANNEL
  NS_DECL_NSIRESUMABLECHANNEL
  NS_DECL_NSIPROXIEDCHANNEL
  NS_DECL_NSICHILDCHANNEL

  NS_IMETHOD Cancel(nsresult status);
  NS_IMETHOD Suspend();
  NS_IMETHOD Resume();

  FTPChannelChild(nsIURI* uri);
  virtual ~FTPChannelChild();

  void AddIPDLReference();
  void ReleaseIPDLReference();

  NS_IMETHOD AsyncOpen(nsIStreamListener* listener, nsISupports* aContext);

  
  
  NS_IMETHOD IsPending(PRBool* result);

  NS_OVERRIDE nsresult OpenContentStream(PRBool async,
                                         nsIInputStream** stream,
                                         nsIChannel** channel);

  bool IsSuspended();

protected:
  NS_OVERRIDE bool RecvOnStartRequest(const PRInt32& aContentLength,
                                      const nsCString& aContentType,
                                      const PRTime& aLastModified,
                                      const nsCString& aEntityID,
                                      const IPC::URI& aURI);
  NS_OVERRIDE bool RecvOnDataAvailable(const nsCString& data,
                                       const PRUint32& offset,
                                       const PRUint32& count);
  NS_OVERRIDE bool RecvOnStopRequest(const nsresult& statusCode);
  NS_OVERRIDE bool RecvCancelEarly(const nsresult& statusCode);
  NS_OVERRIDE bool RecvDeleteSelf();

  void DoOnStartRequest(const PRInt32& aContentLength,
                        const nsCString& aContentType,
                        const PRTime& aLastModified,
                        const nsCString& aEntityID,
                        const IPC::URI& aURI);
  void DoOnDataAvailable(const nsCString& data,
                         const PRUint32& offset,
                         const PRUint32& count);
  void DoOnStopRequest(const nsresult& statusCode);
  void DoCancelEarly(const nsresult& statusCode);
  void DoDeleteSelf();

  friend class FTPStartRequestEvent;
  friend class FTPDataAvailableEvent;
  friend class FTPStopRequestEvent;
  friend class FTPCancelEarlyEvent;
  friend class FTPDeleteSelfEvent;

private:
  nsCOMPtr<nsIInputStream> mUploadStream;

  bool mIPCOpen;
  ChannelEventQueue mEventQ;
  bool mCanceled;
  PRUint32 mSuspendCount;
  PRPackedBool mIsPending;
  PRPackedBool mWasOpened;
  
  PRTime mLastModifiedTime;
  PRUint64 mStartPos;
  nsCString mEntityID;
};

inline bool
FTPChannelChild::IsSuspended()
{
  return mSuspendCount != 0;
}

} 
} 

#endif 
