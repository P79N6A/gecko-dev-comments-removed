




#ifndef mozilla_net_Http2Push_Internal_h
#define mozilla_net_Http2Push_Internal_h

#include "Http2Session.h"
#include "Http2Stream.h"

#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"
#include "nsHttpRequestHead.h"
#include "nsILoadGroup.h"
#include "nsString.h"
#include "PSpdyPush.h"

namespace mozilla {
namespace net {

class Http2PushTransactionBuffer;

class Http2PushedStream final : public Http2Stream
{
public:
  Http2PushedStream(Http2PushTransactionBuffer *aTransaction,
                    Http2Session *aSession,
                    Http2Stream *aAssociatedStream,
                    uint32_t aID);
  virtual ~Http2PushedStream() {}

  bool GetPushComplete();

  
  virtual Http2Stream *GetConsumerStream() override { return mConsumerStream; };

  void SetConsumerStream(Http2Stream *aStream);
  bool GetHashKey(nsCString &key);

  
  nsresult ReadSegments(nsAHttpSegmentReader *,  uint32_t, uint32_t *) override;
  nsresult WriteSegments(nsAHttpSegmentWriter *, uint32_t, uint32_t *) override;

  nsILoadGroupConnectionInfo *LoadGroupConnectionInfo() override { return mLoadGroupCI; };
  void ConnectPushedStream(Http2Stream *consumer);

  bool TryOnPush();

  virtual bool DeferCleanup(nsresult status) override;
  void SetDeferCleanupOnSuccess(bool val) { mDeferCleanupOnSuccess = val; }

  bool IsOrphaned(TimeStamp now);
  void OnPushFailed() { mDeferCleanupOnPush = false; mOnPushFailed = true; }

  nsresult GetBufferedData(char *buf, uint32_t count, uint32_t *countWritten);

  
  virtual bool HasSink() override { return !!mConsumerStream; }

  nsCString &GetRequestString() { return mRequestString; }

private:

  Http2Stream *mConsumerStream; 
                                

  nsCOMPtr<nsILoadGroupConnectionInfo> mLoadGroupCI;

  nsAHttpTransaction *mAssociatedTransaction;

  Http2PushTransactionBuffer *mBufferedPush;
  mozilla::TimeStamp mLastRead;

  nsCString mHashKey;
  nsresult mStatus;
  bool mPushCompleted; 
  bool mDeferCleanupOnSuccess;

  
  
  
  
  
  
  bool mDeferCleanupOnPush;
  bool mOnPushFailed;
  nsCString mRequestString;

};

class Http2PushTransactionBuffer final : public nsAHttpTransaction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION

  Http2PushTransactionBuffer();

  nsresult GetBufferedData(char *buf, uint32_t count, uint32_t *countWritten);
  void SetPushStream(Http2PushedStream *stream) { mPushStream = stream; }

private:
  virtual ~Http2PushTransactionBuffer();

  const static uint32_t kDefaultBufferSize = 4096;

  nsresult mStatus;
  nsHttpRequestHead *mRequestHead;
  Http2PushedStream *mPushStream;
  bool mIsDone;

  nsAutoArrayPtr<char> mBufferedHTTP1;
  uint32_t mBufferedHTTP1Size;
  uint32_t mBufferedHTTP1Used;
  uint32_t mBufferedHTTP1Consumed;
};

} 
} 

#endif 
