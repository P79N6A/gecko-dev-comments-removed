




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

class Http2PushedStream MOZ_FINAL : public Http2Stream
{
public:
  Http2PushedStream(Http2PushTransactionBuffer *aTransaction,
                    Http2Session *aSession,
                    Http2Stream *aAssociatedStream,
                    uint32_t aID);
  virtual ~Http2PushedStream() {}

  bool GetPushComplete();
  Http2Stream *GetConsumerStream() { return mConsumerStream; };
  void SetConsumerStream(Http2Stream *aStream) { mConsumerStream = aStream; }
  bool GetHashKey(nsCString &key);

  
  nsresult ReadSegments(nsAHttpSegmentReader *,  uint32_t, uint32_t *);
  nsresult WriteSegments(nsAHttpSegmentWriter *, uint32_t, uint32_t *);

  nsILoadGroupConnectionInfo *LoadGroupConnectionInfo() { return mLoadGroupCI; };
  void ConnectPushedStream(Http2Stream *consumer);

  bool DeferCleanupOnSuccess() { return mDeferCleanupOnSuccess; }
  void SetDeferCleanupOnSuccess(bool val) { mDeferCleanupOnSuccess = val; }

  bool IsOrphaned(TimeStamp now);

  nsresult GetBufferedData(char *buf, uint32_t count, uint32_t *countWritten);

  
  virtual bool HasSink() { return !!mConsumerStream; }

private:

  Http2Stream *mConsumerStream; 
                                

  nsCOMPtr<nsILoadGroupConnectionInfo> mLoadGroupCI;

  Http2PushTransactionBuffer *mBufferedPush;
  mozilla::TimeStamp mLastRead;

  nsCString mHashKey;
  nsresult mStatus;
  bool mPushCompleted; 
  bool mDeferCleanupOnSuccess;
};

class Http2PushTransactionBuffer MOZ_FINAL : public nsAHttpTransaction
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
