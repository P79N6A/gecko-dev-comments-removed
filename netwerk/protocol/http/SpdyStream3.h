




#ifndef mozilla_net_SpdyStream3_h
#define mozilla_net_SpdyStream3_h

#include "nsAHttpTransaction.h"
#include "mozilla/Attributes.h"

namespace mozilla { namespace net {

class SpdyStream3 MOZ_FINAL : public nsAHttpSegmentReader
                            , public nsAHttpSegmentWriter
{
public:
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  SpdyStream3(nsAHttpTransaction *,
             SpdySession3 *, nsISocketTransport *,
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

  bool HasRegisteredID() { return mStreamID != 0; }

  nsAHttpTransaction *Transaction()
  {
    return mTransaction;
  }

  void Close(nsresult reason);

  void SetRecvdFin(bool aStatus) { mRecvdFin = aStatus ? 1 : 0; }
  bool RecvdFin() { return mRecvdFin; }

  void SetRecvdData(bool aStatus) { mReceivedData = aStatus ? 1 : 0; }
  bool RecvdData() { return mReceivedData; }

  void UpdateTransportSendEvents(uint32_t count);
  void UpdateTransportReadEvents(uint32_t count);

  
  
  static const unsigned char kDictionary[1423];
  static void *zlib_allocator(void *, uInt, uInt);
  static void zlib_destructor(void *, void *);

  nsresult Uncompress(z_stream *, char *, uint32_t);
  nsresult ConvertHeaders(nsACString &);

  void UpdateRemoteWindow(int32_t delta) { mRemoteWindow += delta; }
  int64_t RemoteWindow() { return mRemoteWindow; }

  void DecrementLocalWindow(uint32_t delta) {
    mLocalWindow -= delta;
    mLocalUnacked += delta;
  }

  void IncrementLocalWindow(uint32_t delta) {
    mLocalWindow += delta;
    mLocalUnacked -= delta;
  }

  uint64_t LocalUnAcked() { return mLocalUnacked; }
  bool     BlockedOnRwin() { return mBlockedOnRwin; }

private:

  
  
  
  friend class nsAutoPtr<SpdyStream3>;
  ~SpdyStream3();

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
  void     CompressToFrame(uint32_t);
  void     CompressFlushFrame();
  void     ExecuteCompress(uint32_t);
  nsresult FindHeader(nsCString, nsDependentCSubstring &);
  
  
  
  
  enum stateType mUpstreamState;

  
  
  
  
  nsRefPtr<nsAHttpTransaction> mTransaction;

  
  SpdySession3                *mSession;

  
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

  
  
  uint32_t                     mReceivedData         : 1;

  
  uint32_t                     mSetTCPSocketBuffer   : 1;

  
  
  nsAutoArrayPtr<uint8_t>      mTxInlineFrame;
  uint32_t                     mTxInlineFrameSize;
  uint32_t                     mTxInlineFrameUsed;

  
  
  
  uint32_t                     mTxStreamFrameSize;

  
  
  
  z_stream                     *mZlib;
  nsCString                    mFlatHttpRequestHeaders;

  
  uint32_t             mDecompressBufferSize;
  uint32_t             mDecompressBufferUsed;
  uint32_t             mDecompressedBytes;
  nsAutoArrayPtr<char> mDecompressBuffer;

  
  
  
  
  
  int64_t                      mRequestBodyLenRemaining;

  
  int32_t                      mPriority;

  
  
  

  
  
  int64_t                      mLocalWindow;

  
  
  int64_t                      mRemoteWindow;

  
  
  
  uint64_t                     mLocalUnacked;

  
  
  bool                         mBlockedOnRwin;

  
  uint64_t                     mTotalSent;
  uint64_t                     mTotalRead;
};

}} 

#endif 
