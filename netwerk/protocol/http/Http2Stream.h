




#ifndef mozilla_net_Http2Stream_h
#define mozilla_net_Http2Stream_h

#include "mozilla/Attributes.h"
#include "nsAHttpTransaction.h"
#include "nsISupportsPriority.h"

class nsStandardURL;

namespace mozilla {
namespace net {

class Http2Session;
class Http2Decompressor;

class Http2Stream
  : public nsAHttpSegmentReader
  , public nsAHttpSegmentWriter
{
public:
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  enum stateType {
    IDLE,
    RESERVED_BY_REMOTE,
    OPEN,
    CLOSED_BY_LOCAL,
    CLOSED_BY_REMOTE,
    CLOSED
  };

  const static int32_t kNormalPriority = 0x1000;
  const static int32_t kWorstPriority = kNormalPriority + nsISupportsPriority::PRIORITY_LOWEST;
  const static int32_t kBestPriority = kNormalPriority + nsISupportsPriority::PRIORITY_HIGHEST;

  Http2Stream(nsAHttpTransaction *, Http2Session *, int32_t);

  uint32_t StreamID() { return mStreamID; }
  Http2PushedStream *PushSource() { return mPushSource; }

  stateType HTTPState() { return mState; }
  void SetHTTPState(stateType val) { mState = val; }

  virtual nsresult ReadSegments(nsAHttpSegmentReader *,  uint32_t, uint32_t *);
  virtual nsresult WriteSegments(nsAHttpSegmentWriter *, uint32_t, uint32_t *);
  virtual bool DeferCleanup(nsresult status) { return false; }

  
  
  virtual Http2Stream *GetConsumerStream() { return nullptr; };

  const nsAFlatCString &Origin() const { return mOrigin; }
  const nsAFlatCString &Host() const { return mHeaderHost; }
  const nsAFlatCString &Path() const { return mHeaderPath; }

  bool RequestBlockedOnRead()
  {
    return static_cast<bool>(mRequestBlockedOnRead);
  }

  bool HasRegisteredID() { return mStreamID != 0; }

  nsAHttpTransaction *Transaction() { return mTransaction; }
  virtual nsILoadGroupConnectionInfo *LoadGroupConnectionInfo()
  {
    return mTransaction ? mTransaction->LoadGroupConnectionInfo() : nullptr;
  }

  void Close(nsresult reason);

  void SetRecvdFin(bool aStatus);
  bool RecvdFin() { return mRecvdFin; }

  void SetRecvdData(bool aStatus) { mReceivedData = aStatus ? 1 : 0; }
  bool RecvdData() { return mReceivedData; }

  void SetSentFin(bool aStatus);
  bool SentFin() { return mSentFin; }

  void SetRecvdReset(bool aStatus);
  bool RecvdReset() { return mRecvdReset; }

  void SetSentReset(bool aStatus);
  bool SentReset() { return mSentReset; }

  void SetQueued(bool aStatus) { mQueued = aStatus ? 1 : 0; }
  bool Queued() { return mQueued; }

  void SetCountAsActive(bool aStatus) { mCountAsActive = aStatus ? 1 : 0; }
  bool CountAsActive() { return mCountAsActive; }

  void SetAllHeadersReceived();
  void UnsetAllHeadersReceived() { mAllHeadersReceived = 0; }
  bool AllHeadersReceived() { return mAllHeadersReceived; }

  void UpdateTransportSendEvents(uint32_t count);
  void UpdateTransportReadEvents(uint32_t count);

  
  nsresult ConvertResponseHeaders(Http2Decompressor *, nsACString &,
                                  nsACString &, int32_t &);
  nsresult ConvertPushHeaders(Http2Decompressor *, nsACString &, nsACString &);

  bool AllowFlowControlledWrite();
  void UpdateServerReceiveWindow(int32_t delta);
  int64_t ServerReceiveWindow() { return mServerReceiveWindow; }

  void DecrementClientReceiveWindow(uint32_t delta) {
    mClientReceiveWindow -= delta;
    mLocalUnacked += delta;
  }

  void IncrementClientReceiveWindow(uint32_t delta) {
    mClientReceiveWindow += delta;
    mLocalUnacked -= delta;
  }

  uint64_t LocalUnAcked() { return mLocalUnacked; }
  int64_t  ClientReceiveWindow()  { return mClientReceiveWindow; }

  bool     BlockedOnRwin() { return mBlockedOnRwin; }

  uint32_t Priority() { return mPriority; }
  void SetPriority(uint32_t);
  void SetPriorityDependency(uint32_t, uint8_t, bool);
  void UpdatePriorityDependency();

  
  
  virtual bool HasSink() { return true; }

  virtual ~Http2Stream();

  Http2Session *Session() { return mSession; }

  static nsresult MakeOriginURL(const nsACString &origin,
                                nsRefPtr<nsStandardURL> &url);

  static nsresult MakeOriginURL(const nsACString &scheme,
                                const nsACString &origin,
                                nsRefPtr<nsStandardURL> &url);

protected:
  static void CreatePushHashKey(const nsCString &scheme,
                                const nsCString &hostHeader,
                                uint64_t serial,
                                const nsCSubstring &pathInfo,
                                nsCString &outOrigin,
                                nsCString &outKey);

  
  enum upstreamStateType {
    GENERATING_HEADERS,
    GENERATING_BODY,
    SENDING_BODY,
    SENDING_FIN_STREAM,
    UPSTREAM_COMPLETE
  };

  uint32_t mStreamID;

  
  Http2Session *mSession;

  nsCString     mOrigin;
  nsCString     mHeaderHost;
  nsCString     mHeaderScheme;
  nsCString     mHeaderPath;

  
  
  
  enum upstreamStateType mUpstreamState;

  
  enum stateType mState;

  
  uint32_t                     mRequestHeadersDone   : 1;

  
  uint32_t                     mOpenGenerated        : 1;

  
  uint32_t                     mAllHeadersReceived   : 1;

  
  
  uint32_t                     mQueued               : 1;

  void     ChangeState(enum upstreamStateType);

private:
  friend class nsAutoPtr<Http2Stream>;

  nsresult ParseHttpRequestHeaders(const char *, uint32_t, uint32_t *);
  nsresult GenerateOpen();

  void     AdjustPushedPriority();
  void     AdjustInitialWindow();
  nsresult TransmitFrame(const char *, uint32_t *, bool forceCommitment);
  void     GenerateDataFrameHeader(uint32_t, bool);

  
  
  
  
  nsRefPtr<nsAHttpTransaction> mTransaction;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader        *mSegmentReader;
  nsAHttpSegmentWriter        *mSegmentWriter;

  
  uint32_t                    mChunkSize;

  
  
  uint32_t                     mRequestBlockedOnRead : 1;

  
  
  uint32_t                     mRecvdFin             : 1;

  
  uint32_t                     mReceivedData         : 1;

  
  uint32_t                     mRecvdReset           : 1;

  
  uint32_t                     mSentReset            : 1;

  
  uint32_t                     mCountAsActive        : 1;

  
  
  uint32_t                     mSentFin              : 1;

  
  uint32_t                     mSentWaitingFor       : 1;

  
  uint32_t                     mSetTCPSocketBuffer   : 1;

  
  
  nsAutoArrayPtr<uint8_t>      mTxInlineFrame;
  uint32_t                     mTxInlineFrameSize;
  uint32_t                     mTxInlineFrameUsed;

  
  
  
  uint32_t                     mTxStreamFrameSize;

  
  nsCString                    mFlatHttpRequestHeaders;

  
  
  
  
  
  
  int64_t                      mRequestBodyLenRemaining;

  uint32_t                     mPriority; 
  uint32_t                     mPriorityDependency; 
  uint8_t                      mPriorityWeight; 

  
  
  

  
  
  int64_t                      mClientReceiveWindow;

  
  
  int64_t                      mServerReceiveWindow;

  
  
  
  uint64_t                     mLocalUnacked;

  
  
  bool                         mBlockedOnRwin;

  
  uint64_t                     mTotalSent;
  uint64_t                     mTotalRead;

  
  Http2PushedStream *mPushSource;


public:
  bool IsTunnel() { return mIsTunnel; }
private:
  void ClearTransactionsBlockedOnTunnel();
  void MapStreamToPlainText();
  void MapStreamToHttpConnection();

  bool mIsTunnel;
  bool mPlainTextTunnel;
};

} 
} 

#endif 
