




#ifndef mozilla_net_Http2Session_h
#define mozilla_net_Http2Session_h



#include "ASpdySession.h"
#include "mozilla/Attributes.h"
#include "nsAHttpConnection.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsDeque.h"
#include "nsHashKeys.h"

#include "Http2Compression.h"

class nsISocketTransport;

namespace mozilla {
namespace net {

class Http2PushedStream;
class Http2Stream;
class nsHttpTransaction;

class Http2Session MOZ_FINAL : public ASpdySession
  , public nsAHttpConnection
  , public nsAHttpSegmentReader
  , public nsAHttpSegmentWriter
{
  ~Http2Session();

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION
  NS_DECL_NSAHTTPCONNECTION(mConnection)
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  explicit Http2Session(nsISocketTransport *);

  bool AddStream(nsAHttpTransaction *, int32_t,
                 bool, nsIInterfaceRequestor *);
  bool CanReuse() { return !mShouldGoAway && !mClosed; }
  bool RoomForMoreStreams();

  
  
  
  
  uint32_t  ReadTimeoutTick(PRIntervalTime now);

  
  PRIntervalTime IdleTime();

  
  uint32_t RegisterStreamID(Http2Stream *, uint32_t aNewID = 0);















  enum frameType {
    FRAME_TYPE_DATA = 0,
    FRAME_TYPE_HEADERS = 1,
    FRAME_TYPE_PRIORITY = 2,
    FRAME_TYPE_RST_STREAM = 3,
    FRAME_TYPE_SETTINGS = 4,
    FRAME_TYPE_PUSH_PROMISE = 5,
    FRAME_TYPE_PING = 6,
    FRAME_TYPE_GOAWAY = 7,
    FRAME_TYPE_WINDOW_UPDATE = 8,
    FRAME_TYPE_CONTINUATION = 9,
    FRAME_TYPE_LAST = 10
  };

  
  
  enum errorType {
    NO_HTTP_ERROR = 0,
    PROTOCOL_ERROR = 1,
    INTERNAL_ERROR = 2,
    FLOW_CONTROL_ERROR = 3,
    SETTINGS_TIMEOUT_ERROR = 4,
    STREAM_CLOSED_ERROR = 5,
    FRAME_SIZE_ERROR = 6,
    REFUSED_STREAM_ERROR = 7,
    CANCEL_ERROR = 8,
    COMPRESSION_ERROR = 9,
    CONNECT_ERROR = 10,
    ENHANCE_YOUR_CALM = 11,
    INADEQUATE_SECURITY = 12
  };

  
  
  const static uint8_t kFlag_END_STREAM = 0x01; 
  const static uint8_t kFlag_END_HEADERS = 0x04; 
  const static uint8_t kFlag_END_PUSH_PROMISE = 0x04; 
  const static uint8_t kFlag_ACK = 0x01; 
  const static uint8_t kFlag_PADDED = 0x08; 
  const static uint8_t kFlag_PRIORITY = 0x20; 

  enum {
    SETTINGS_TYPE_HEADER_TABLE_SIZE = 1, 
    SETTINGS_TYPE_ENABLE_PUSH = 2,     
    SETTINGS_TYPE_MAX_CONCURRENT = 3,  
    SETTINGS_TYPE_INITIAL_WINDOW = 4,  
    SETTINGS_TYPE_MAX_FRAME_SIZE = 5   
  };

  
  
  const static uint32_t kDefaultBufferSize = 2048;

  
  const static uint32_t kDefaultQueueSize =  32768;
  const static uint32_t kQueueMinimumCleanup = 24576;
  const static uint32_t kQueueTailRoom    =  4096;
  const static uint32_t kQueueReserved    =  1024;

  const static uint32_t kDefaultMaxConcurrent = 100;
  const static uint32_t kMaxStreamID = 0x7800000;

  
  
  const static uint32_t kDeadStreamID = 0xffffdead;

  
  
  const static int32_t  kEmergencyWindowThreshold = 256 * 1024;
  const static uint32_t kMinimumToAck = 4 * 1024 * 1024;

  
  const static uint32_t kDefaultRwin = 65535;

  
  
  const static uint32_t kMaxFrameData = 0x4000;

  const static uint8_t kFrameLengthBytes = 3;
  const static uint8_t kFrameStreamIDBytes = 4;
  const static uint8_t kFrameFlagBytes = 1;
  const static uint8_t kFrameTypeBytes = 1;
  const static uint8_t kFrameHeaderBytes = kFrameLengthBytes + kFrameFlagBytes +
    kFrameTypeBytes + kFrameStreamIDBytes;

  static nsresult RecvHeaders(Http2Session *);
  static nsresult RecvPriority(Http2Session *);
  static nsresult RecvRstStream(Http2Session *);
  static nsresult RecvSettings(Http2Session *);
  static nsresult RecvPushPromise(Http2Session *);
  static nsresult RecvPing(Http2Session *);
  static nsresult RecvGoAway(Http2Session *);
  static nsresult RecvWindowUpdate(Http2Session *);
  static nsresult RecvContinuation(Http2Session *);

  char       *EnsureOutputBuffer(uint32_t needed);

  template<typename charType>
  void CreateFrameHeader(charType dest, uint16_t frameLength,
                         uint8_t frameType, uint8_t frameFlags,
                         uint32_t streamID);

  
  static void LogIO(Http2Session *, Http2Stream *, const char *,
                    const char *, uint32_t);

  
  void TransactionHasDataToWrite(nsAHttpTransaction *);

  
  void TransactionHasDataToWrite(Http2Stream *);

  
  virtual nsresult CommitToSegmentSize(uint32_t size, bool forceCommitment);
  nsresult BufferOutput(const char *, uint32_t, uint32_t *);
  void     FlushOutputQueue();
  uint32_t AmountOfOutputBuffered() { return mOutputQueueUsed - mOutputQueueSent; }

  uint32_t GetServerInitialStreamWindow() { return mServerInitialStreamWindow; }

  void ConnectPushedStream(Http2Stream *stream);
  void MaybeDecrementConcurrent(Http2Stream *stream);

  nsresult ConfirmTLSProfile();
  static bool ALPNCallback(nsISupports *securityInfo);

  uint64_t Serial() { return mSerial; }

  void PrintDiagnostics (nsCString &log);

  
  uint32_t SendingChunkSize() { return mSendingChunkSize; }
  uint32_t PushAllowance() { return mPushAllowance; }
  Http2Compressor *Compressor() { return &mCompressor; }
  nsISocketTransport *SocketTransport() { return mSocketTransport; }
  int64_t ServerSessionWindow() { return mServerSessionWindow; }
  void DecrementServerSessionWindow (uint32_t bytes) { mServerSessionWindow -= bytes; }

private:

  
  enum internalStateType {
    BUFFERING_OPENING_SETTINGS,
    BUFFERING_FRAME_HEADER,
    BUFFERING_CONTROL_FRAME,
    PROCESSING_DATA_FRAME_PADDING_CONTROL,
    PROCESSING_DATA_FRAME,
    DISCARDING_DATA_FRAME_PADDING,
    DISCARDING_DATA_FRAME,
    PROCESSING_COMPLETE_HEADERS,
    PROCESSING_CONTROL_RST_STREAM
  };

  static const uint8_t kMagicHello[24];

  nsresult    ResponseHeadersComplete();
  uint32_t    GetWriteQueueSize();
  void        ChangeDownstreamState(enum internalStateType);
  void        ResetDownstreamState();
  nsresult    ReadyToProcessDataFrame(enum internalStateType);
  nsresult    UncompressAndDiscard();
  void        GeneratePing(bool);
  void        GenerateSettingsAck();
  void        GeneratePriority(uint32_t, uint8_t);
  void        GenerateRstStream(uint32_t, uint32_t);
  void        GenerateGoAway(uint32_t);
  void        CleanupStream(Http2Stream *, nsresult, errorType);
  void        CloseStream(Http2Stream *, nsresult);
  void        SendHello();
  void        RemoveStreamFromQueues(Http2Stream *);
  nsresult    ParsePadding(uint8_t &, uint16_t &);

  void        SetWriteCallbacks();
  void        RealignOutputQueue();

  bool        RoomForMoreConcurrent();
  void        ActivateStream(Http2Stream *);
  void        ProcessPending();
  nsresult    SetInputFrameDataStream(uint32_t);
  bool        VerifyStream(Http2Stream *, uint32_t);
  void        SetNeedsCleanup();

  void        UpdateLocalRwin(Http2Stream *stream, uint32_t bytes);
  void        UpdateLocalStreamWindow(Http2Stream *stream, uint32_t bytes);
  void        UpdateLocalSessionWindow(uint32_t bytes);

  
  
  nsresult   NetworkRead(nsAHttpSegmentWriter *, char *, uint32_t, uint32_t *);

  static PLDHashOperator ShutdownEnumerator(nsAHttpTransaction *,
                                            nsAutoPtr<Http2Stream> &,
                                            void *);

  static PLDHashOperator GoAwayEnumerator(nsAHttpTransaction *,
                                          nsAutoPtr<Http2Stream> &,
                                          void *);

  static PLDHashOperator UpdateServerRwinEnumerator(nsAHttpTransaction *,
                                                    nsAutoPtr<Http2Stream> &,
                                                    void *);

  static PLDHashOperator RestartBlockedOnRwinEnumerator(nsAHttpTransaction *,
                                                        nsAutoPtr<Http2Stream> &,
                                                        void *);

  
  
  
  nsRefPtr<nsAHttpConnection> mConnection;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader       *mSegmentReader;
  nsAHttpSegmentWriter       *mSegmentWriter;

  uint32_t          mSendingChunkSize;        
  uint32_t          mNextStreamID;            
  uint32_t          mConcurrentHighWater;     
  uint32_t          mPushAllowance;           

  internalStateType mDownstreamState; 

  
  
  
  
  
  
  
  nsDataHashtable<nsUint32HashKey, Http2Stream *>     mStreamIDHash;
  nsClassHashtable<nsPtrHashKey<nsAHttpTransaction>,
    Http2Stream>                                      mStreamTransactionHash;

  nsDeque                                             mReadyForWrite;
  nsDeque                                             mQueuedStreams;
  nsDeque                                             mReadyForRead;
  nsTArray<Http2PushedStream *>                       mPushedStreams;

  
  
  
  
  
  Http2Compressor     mCompressor;
  Http2Decompressor   mDecompressor;
  nsCString           mDecompressBuffer;

  
  
  uint32_t             mInputFrameBufferSize; 
  uint32_t             mInputFrameBufferUsed; 
  nsAutoArrayPtr<char> mInputFrameBuffer;

  
  
  
  
  
  
  uint32_t             mInputFrameDataSize;
  uint32_t             mInputFrameDataRead;
  bool                 mInputFrameFinal; 
  uint8_t              mInputFrameType;
  uint8_t              mInputFrameFlags;
  uint32_t             mInputFrameID;
  uint16_t             mPaddingLength;

  
  
  
  Http2Stream          *mInputFrameDataStream;

  
  
  
  
  
  Http2Stream          *mNeedsCleanup;

  
  uint32_t             mDownstreamRstReason;

  
  
  uint32_t             mExpectedHeaderID;
  uint32_t             mExpectedPushPromiseID;
  uint32_t             mContinuedPromiseStream;

  
  
  nsCString            mFlatHTTPResponseHeaders;
  uint32_t             mFlatHTTPResponseHeadersOut;

  
  
  
  
  
  bool                 mShouldGoAway;

  
  bool                 mClosed;

  
  bool                 mCleanShutdown;

  
  
  bool                 mTLSProfileConfirmed;

  
  
  errorType            mGoAwayReason;

  
  
  uint32_t             mGoAwayID;

  
  uint32_t             mOutgoingGoAwayID;

  
  
  
  uint32_t             mMaxConcurrent;

  
  
  
  uint32_t             mConcurrent;

  
  uint32_t             mServerPushedResources;

  
  uint32_t             mServerInitialStreamWindow;

  
  
  
  int64_t              mLocalSessionWindow;

  
  
  
  int64_t              mServerSessionWindow;

  
  
  
  
  
  uint32_t             mOutputQueueSize;
  uint32_t             mOutputQueueUsed;
  uint32_t             mOutputQueueSent;
  nsAutoArrayPtr<char> mOutputQueueBuffer;

  PRIntervalTime       mPingThreshold;
  PRIntervalTime       mLastReadEpoch;     
  PRIntervalTime       mLastDataReadEpoch; 
  PRIntervalTime       mPingSentEpoch;

  
  nsDeque  mGoAwayStreamsToRestart;

  
  
  
  uint64_t        mSerial;

  
  
  
  
  bool mWaitingForSettingsAck;
  bool mGoAwayOnPush;

private:

  void DispatchOnTunnel(nsAHttpTransaction *, nsIInterfaceRequestor *);
  void RegisterTunnel(Http2Stream *);
  void UnRegisterTunnel(Http2Stream *);
  uint32_t FindTunnelCount(nsHttpConnectionInfo *);

  nsDataHashtable<nsCStringHashKey, uint32_t> mTunnelHash;
};

} 
} 

#endif
