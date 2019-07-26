





#ifndef mozilla_net_SpdyStream2_h
#define mozilla_net_SpdyStream2_h

#include "nsAHttpTransaction.h"
#include "mozilla/Attributes.h"

namespace mozilla { namespace net {

class SpdyStream2 MOZ_FINAL : public nsAHttpSegmentReader
                            , public nsAHttpSegmentWriter
{
public:
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  SpdyStream2(nsAHttpTransaction *,
             SpdySession2 *, nsISocketTransport *,
             uint32_t, z_stream *, int32_t);

  uint32_t StreamID() { return mStreamID; }

  nsresult ReadSegments(nsAHttpSegmentReader *,  uint32_t, uint32_t *);
  nsresult WriteSegments(nsAHttpSegmentWriter *, uint32_t, uint32_t *);

  bool RequestBlockedOnRead()
  {
    return static_cast<bool>(mRequestBlockedOnRead);
  }

  
  bool GetFullyOpen() {return mFullyOpen;}
  void SetFullyOpen() 
  {
    NS_ABORT_IF_FALSE(!mFullyOpen, "SetFullyOpen already open");
    mFullyOpen = 1;
  }

  nsAHttpTransaction *Transaction()
  {
    return mTransaction;
  }

  void Close(nsresult reason);

  void SetRecvdFin(bool aStatus) { mRecvdFin = aStatus ? 1 : 0; }
  bool RecvdFin() { return mRecvdFin; }

  void UpdateTransportSendEvents(uint32_t count);
  void UpdateTransportReadEvents(uint32_t count);

  
  
  static const char *kDictionary;
  static void *zlib_allocator(void *, uInt, uInt);
  static void zlib_destructor(void *, void *);

private:

  
  
  
  friend class nsAutoPtr<SpdyStream2>;
  ~SpdyStream2();

  enum stateType {
    GENERATING_SYN_STREAM,
    GENERATING_REQUEST_BODY,
    SENDING_REQUEST_BODY,
    SENDING_FIN_STREAM,
    UPSTREAM_COMPLETE
  };

  static PLDHashOperator hdrHashEnumerate(const nsACString &,
                                          nsAutoPtr<nsCString> &,
                                          void *);

  void     ChangeState(enum stateType);
  nsresult ParseHttpRequestHeaders(const char *, uint32_t, uint32_t *);
  nsresult TransmitFrame(const char *, uint32_t *, bool forceCommitment);
  void     GenerateDataFrameHeader(uint32_t, bool);

  void     CompressToFrame(const nsACString &);
  void     CompressToFrame(const nsACString *);
  void     CompressToFrame(const char *, uint32_t);
  void     CompressToFrame(uint16_t);
  void     CompressFlushFrame();
  void     ExecuteCompress(uint32_t);
  
  
  
  
  enum stateType mUpstreamState;

  
  
  
  
  nsRefPtr<nsAHttpTransaction> mTransaction;

  
  SpdySession2                *mSession;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader        *mSegmentReader;
  nsAHttpSegmentWriter        *mSegmentWriter;

  
  uint32_t                    mStreamID;

  
  uint32_t                    mChunkSize;

  
  uint32_t                     mSynFrameComplete     : 1;

  
  
  uint32_t                     mRequestBlockedOnRead : 1;

  
  
  uint32_t                     mSentFinOnData        : 1;

  
  
  uint32_t                     mRecvdFin             : 1;

  
  uint32_t                     mFullyOpen            : 1;

  
  uint32_t                     mSentWaitingFor       : 1;

  
  uint32_t                     mSetTCPSocketBuffer   : 1;

  
  
  nsAutoArrayPtr<char>         mTxInlineFrame;
  uint32_t                     mTxInlineFrameSize;
  uint32_t                     mTxInlineFrameUsed;

  
  
  
  uint32_t                     mTxStreamFrameSize;

  
  
  
  z_stream                     *mZlib;
  nsCString                    mFlatHttpRequestHeaders;

  
  
  
  
  
  int64_t                      mRequestBodyLenRemaining;

  
  int32_t                      mPriority;

  
  uint64_t                     mTotalSent;
  uint64_t                     mTotalRead;
};

}} 

#endif 
