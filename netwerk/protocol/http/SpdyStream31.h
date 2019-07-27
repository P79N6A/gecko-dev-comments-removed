




#ifndef mozilla_net_SpdyStream31_h
#define mozilla_net_SpdyStream31_h

#include "mozilla/Attributes.h"
#include "nsAHttpTransaction.h"

namespace mozilla { namespace net {

class SpdyStream31 : public nsAHttpSegmentReader
                   , public nsAHttpSegmentWriter
{
public:
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  SpdyStream31(nsAHttpTransaction *, SpdySession31 *, int32_t);

  uint32_t StreamID() { return mStreamID; }
  SpdyPushedStream31 *PushSource() { return mPushSource; }

  virtual nsresult ReadSegments(nsAHttpSegmentReader *,  uint32_t, uint32_t *);
  virtual nsresult WriteSegments(nsAHttpSegmentWriter *, uint32_t, uint32_t *);
  virtual bool DeferCleanupOnSuccess() { return false; }

  const nsAFlatCString &Origin()   const { return mOrigin; }

  bool RequestBlockedOnRead()
  {
    return static_cast<bool>(mRequestBlockedOnRead);
  }

  bool GetFullyOpen();
  
  
  nsresult SetFullyOpen();

  bool HasRegisteredID() { return mStreamID != 0; }

  nsAHttpTransaction *Transaction() { return mTransaction; }
  virtual nsISchedulingContext *SchedulingContext()
  {
    return mTransaction ? mTransaction->SchedulingContext() : nullptr;
  }

  void Close(nsresult reason);

  void SetRecvdFin(bool aStatus) { mRecvdFin = aStatus ? 1 : 0; }
  bool RecvdFin() { return mRecvdFin; }

  void SetRecvdData(bool aStatus) { mReceivedData = aStatus ? 1 : 0; }
  bool RecvdData() { return mReceivedData; }

  void SetQueued(bool aStatus) { mQueued = aStatus ? 1 : 0; }
  bool Queued() { return mQueued; }

  void SetCountAsActive(bool aStatus) { mCountAsActive = aStatus ? 1 : 0; }
  bool CountAsActive() { return mCountAsActive; }

  void UpdateTransportSendEvents(uint32_t count);
  void UpdateTransportReadEvents(uint32_t count);

  
  static const unsigned char kDictionary[1423];

  nsresult Uncompress(z_stream *, char *, uint32_t);
  nsresult ConvertHeaders(nsACString &);

  void UpdateRemoteWindow(int32_t delta);
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
  int64_t  LocalWindow()  { return mLocalWindow; }

  bool     BlockedOnRwin() { return mBlockedOnRwin; }

  
  
  virtual bool HasSink() { return true; }

  virtual ~SpdyStream31();

protected:
  nsresult FindHeader(nsCString, nsDependentCSubstring &);

  static void CreatePushHashKey(const nsCString &scheme,
                                const nsCString &hostHeader,
                                uint64_t serial,
                                const nsCSubstring &pathInfo,
                                nsCString &outOrigin,
                                nsCString &outKey);

  enum stateType {
    GENERATING_SYN_STREAM,
    GENERATING_REQUEST_BODY,
    SENDING_REQUEST_BODY,
    SENDING_FIN_STREAM,
    UPSTREAM_COMPLETE
  };

  uint32_t mStreamID;

  
  SpdySession31 *mSession;

  nsCString     mOrigin;

  
  
  
  enum stateType mUpstreamState;

  
  uint32_t                     mRequestHeadersDone   : 1;

  
  uint32_t                     mSynFrameGenerated    : 1;

  
  
  uint32_t                     mSentFinOnData        : 1;

  
  
  uint32_t                     mQueued               : 1;

  void     ChangeState(enum stateType);

private:
  friend class nsAutoPtr<SpdyStream31>;

  static PLDHashOperator hdrHashEnumerate(const nsACString &,
                                          nsAutoPtr<nsCString> &,
                                          void *);

  nsresult ParseHttpRequestHeaders(const char *, uint32_t, uint32_t *);
  nsresult GenerateSynFrame();

  void     AdjustInitialWindow();
  nsresult TransmitFrame(const char *, uint32_t *, bool forceCommitment);
  void     GenerateDataFrameHeader(uint32_t, bool);

  void     CompressToFrame(const nsACString &);
  void     CompressToFrame(const nsACString *);
  void     CompressToFrame(const char *, uint32_t);
  void     CompressToFrame(uint32_t);
  void     CompressFlushFrame();
  void     ExecuteCompress(uint32_t);

  
  
  
  
  nsRefPtr<nsAHttpTransaction> mTransaction;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader        *mSegmentReader;
  nsAHttpSegmentWriter        *mSegmentWriter;

  
  uint32_t                    mChunkSize;

  
  
  uint32_t                     mRequestBlockedOnRead : 1;

  
  
  uint32_t                     mRecvdFin             : 1;

  
  uint32_t                     mFullyOpen            : 1;

  
  uint32_t                     mSentWaitingFor       : 1;

  
  
  uint32_t                     mReceivedData         : 1;

  
  uint32_t                     mSetTCPSocketBuffer   : 1;

  
  uint32_t                     mCountAsActive        : 1;

  
  
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

  
  SpdyPushedStream31 *mPushSource;


public:
  bool IsTunnel() { return mIsTunnel; }
private:
  void ClearTransactionsBlockedOnTunnel();
  void MapStreamToPlainText();
  void MapStreamToHttpConnection();

  bool mIsTunnel;
  bool mPlainTextTunnel;
};

}} 

#endif 
