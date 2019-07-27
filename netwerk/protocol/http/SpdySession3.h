




#ifndef mozilla_net_SpdySession3_h
#define mozilla_net_SpdySession3_h




#include "ASpdySession.h"
#include "mozilla/Attributes.h"
#include "nsAHttpConnection.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsDeque.h"
#include "nsHashKeys.h"
#include "zlib.h"

class nsISocketTransport;

namespace mozilla { namespace net {

class SpdyPushedStream3;
class SpdyStream3;
class nsHttpTransaction;

class SpdySession3 MOZ_FINAL : public ASpdySession
                             , public nsAHttpConnection
                             , public nsAHttpSegmentReader
                             , public nsAHttpSegmentWriter
{
  ~SpdySession3();

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION
  NS_DECL_NSAHTTPCONNECTION(mConnection)
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  explicit SpdySession3(nsISocketTransport *);

  bool AddStream(nsAHttpTransaction *, int32_t,
                 bool, nsIInterfaceRequestor *);
  bool CanReuse() { return !mShouldGoAway && !mClosed; }
  bool RoomForMoreStreams();

  
  
  
  
  uint32_t  ReadTimeoutTick(PRIntervalTime now);

  
  PRIntervalTime IdleTime();

  
  uint32_t RegisterStreamID(SpdyStream3 *, uint32_t aNewID = 0);

  const static uint8_t kVersion        = 3;

  const static uint8_t kFlag_Control   = 0x80;

  const static uint8_t kFlag_Data_FIN  = 0x01;
  const static uint8_t kFlag_Data_UNI  = 0x02;

  enum
  {
    CONTROL_TYPE_FIRST = 0,
    CONTROL_TYPE_SYN_STREAM = 1,
    CONTROL_TYPE_SYN_REPLY = 2,
    CONTROL_TYPE_RST_STREAM = 3,
    CONTROL_TYPE_SETTINGS = 4,
    CONTROL_TYPE_NOOP = 5,                        
    CONTROL_TYPE_PING = 6,
    CONTROL_TYPE_GOAWAY = 7,
    CONTROL_TYPE_HEADERS = 8,
    CONTROL_TYPE_WINDOW_UPDATE = 9,
    CONTROL_TYPE_CREDENTIAL = 10,
    CONTROL_TYPE_LAST = 11
  };

  enum rstReason
  {
    RST_PROTOCOL_ERROR = 1,
    RST_INVALID_STREAM = 2,
    RST_REFUSED_STREAM = 3,
    RST_UNSUPPORTED_VERSION = 4,
    RST_CANCEL = 5,
    RST_INTERNAL_ERROR = 6,
    RST_FLOW_CONTROL_ERROR = 7,
    RST_STREAM_IN_USE = 8,
    RST_STREAM_ALREADY_CLOSED = 9,
    RST_INVALID_CREDENTIALS = 10,
    RST_FRAME_TOO_LARGE = 11
  };

  enum goawayReason
  {
    OK = 0,
    PROTOCOL_ERROR = 1,
    INTERNAL_ERROR = 2,    
    NUM_STATUS_CODES = 3   
  };

  enum settingsFlags
  {
    PERSIST_VALUE = 1,
    PERSISTED_VALUE = 2
  };

  enum
  {
    SETTINGS_TYPE_UPLOAD_BW = 1, 
    SETTINGS_TYPE_DOWNLOAD_BW = 2, 
    SETTINGS_TYPE_RTT = 3, 
    SETTINGS_TYPE_MAX_CONCURRENT = 4, 
    SETTINGS_TYPE_CWND = 5, 
    SETTINGS_TYPE_DOWNLOAD_RETRANS_RATE = 6, 
    SETTINGS_TYPE_INITIAL_WINDOW = 7,  
    SETTINGS_CLIENT_CERTIFICATE_VECTOR_SIZE = 8
  };

  
  
  
  
  const static uint32_t kDefaultBufferSize = 2048;

  
  const static uint32_t kDefaultQueueSize =  32768;
  const static uint32_t kQueueMinimumCleanup = 24576;
  const static uint32_t kQueueTailRoom    =  4096;
  const static uint32_t kQueueReserved    =  1024;

  const static uint32_t kDefaultMaxConcurrent = 100;
  const static uint32_t kMaxStreamID = 0x7800000;

  
  
  const static uint32_t kDeadStreamID = 0xffffdead;

  
  
  const static int32_t  kEmergencyWindowThreshold = 1024 * 1024;
  const static uint32_t kMinimumToAck = 64 * 1024;

  
  const static uint32_t kDefaultServerRwin = 64 * 1024;

  static nsresult HandleSynStream(SpdySession3 *);
  static nsresult HandleSynReply(SpdySession3 *);
  static nsresult HandleRstStream(SpdySession3 *);
  static nsresult HandleSettings(SpdySession3 *);
  static nsresult HandleNoop(SpdySession3 *);
  static nsresult HandlePing(SpdySession3 *);
  static nsresult HandleGoAway(SpdySession3 *);
  static nsresult HandleHeaders(SpdySession3 *);
  static nsresult HandleWindowUpdate(SpdySession3 *);
  static nsresult HandleCredential(SpdySession3 *);

  
  static void LogIO(SpdySession3 *, SpdyStream3 *, const char *,
                    const char *, uint32_t);

  
  void TransactionHasDataToWrite(nsAHttpTransaction *);

  
  void TransactionHasDataToWrite(SpdyStream3 *);

  
  virtual nsresult CommitToSegmentSize(uint32_t size, bool forceCommitment);

  uint32_t GetServerInitialWindow() { return mServerInitialWindow; }

  void ConnectPushedStream(SpdyStream3 *stream);
  void DecrementConcurrent(SpdyStream3 *stream);

  uint64_t Serial() { return mSerial; }

  void     PrintDiagnostics (nsCString &log);

  
  uint32_t SendingChunkSize() { return mSendingChunkSize; }
  uint32_t PushAllowance() { return mPushAllowance; }
  z_stream *UpstreamZlib() { return &mUpstreamZlib; }
  nsISocketTransport *SocketTransport() { return mSocketTransport; }

  void SendPing() MOZ_OVERRIDE;

private:

  enum stateType {
    BUFFERING_FRAME_HEADER,
    BUFFERING_CONTROL_FRAME,
    PROCESSING_DATA_FRAME,
    DISCARDING_DATA_FRAME,
    PROCESSING_COMPLETE_HEADERS,
    PROCESSING_CONTROL_RST_STREAM
  };

  nsresult    ResponseHeadersComplete();
  uint32_t    GetWriteQueueSize();
  void        ChangeDownstreamState(enum stateType);
  void        ResetDownstreamState();
  nsresult    UncompressAndDiscard(uint32_t, uint32_t);
  void        zlibInit();
  void        GeneratePing(uint32_t);
  void        GenerateRstStream(uint32_t, uint32_t);
  void        GenerateGoAway(uint32_t);
  void        CleanupStream(SpdyStream3 *, nsresult, rstReason);
  void        CloseStream(SpdyStream3 *, nsresult);
  void        GenerateSettings();
  void        RemoveStreamFromQueues(SpdyStream3 *);

  void        SetWriteCallbacks();
  void        FlushOutputQueue();
  void        RealignOutputQueue();

  bool        RoomForMoreConcurrent();
  void        ActivateStream(SpdyStream3 *);
  void        ProcessPending();
  nsresult    SetInputFrameDataStream(uint32_t);
  bool        VerifyStream(SpdyStream3 *, uint32_t);
  void        SetNeedsCleanup();

  void        UpdateLocalRwin(SpdyStream3 *stream, uint32_t bytes);

  
  
  nsresult   NetworkRead(nsAHttpSegmentWriter *, char *, uint32_t, uint32_t *);

  static PLDHashOperator ShutdownEnumerator(nsAHttpTransaction *,
                                            nsAutoPtr<SpdyStream3> &,
                                            void *);

  static PLDHashOperator GoAwayEnumerator(nsAHttpTransaction *,
                                          nsAutoPtr<SpdyStream3> &,
                                          void *);

  static PLDHashOperator UpdateServerRwinEnumerator(nsAHttpTransaction *,
                                                    nsAutoPtr<SpdyStream3> &,
                                                    void *);

  
  
  
  nsRefPtr<nsAHttpConnection> mConnection;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader       *mSegmentReader;
  nsAHttpSegmentWriter       *mSegmentWriter;

  uint32_t          mSendingChunkSize;        
  uint32_t          mNextStreamID;            
  uint32_t          mConcurrentHighWater;     
  uint32_t          mPushAllowance;           

  stateType         mDownstreamState; 

  
  
  
  
  
  
  
  nsDataHashtable<nsUint32HashKey, SpdyStream3 *>     mStreamIDHash;
  nsClassHashtable<nsPtrHashKey<nsAHttpTransaction>,
                   SpdyStream3>                       mStreamTransactionHash;

  nsDeque                                             mReadyForWrite;
  nsDeque                                             mQueuedStreams;
  nsDeque                                             mReadyForRead;
  nsTArray<SpdyPushedStream3 *>                       mPushedStreams;

  
  
  
  
  
  z_stream            mDownstreamZlib;
  z_stream            mUpstreamZlib;

  
  
  uint32_t             mInputFrameBufferSize;
  uint32_t             mInputFrameBufferUsed;
  nsAutoArrayPtr<char> mInputFrameBuffer;

  
  
  
  uint32_t             mInputFrameDataSize;
  uint32_t             mInputFrameDataRead;
  bool                 mInputFrameDataLast; 

  
  
  
  SpdyStream3          *mInputFrameDataStream;

  
  
  
  
  
  SpdyStream3          *mNeedsCleanup;

  
  uint32_t             mFrameControlType;

  
  uint32_t             mDownstreamRstReason;

  
  
  nsCString            mFlatHTTPResponseHeaders;
  uint32_t             mFlatHTTPResponseHeadersOut;

  
  
  
  
  
  bool                 mShouldGoAway;

  
  bool                 mClosed;

  
  bool                 mCleanShutdown;

  
  
  
  bool                 mDataPending;

  
  
  uint32_t             mGoAwayID;

  
  
  
  uint32_t             mMaxConcurrent;

  
  
  
  uint32_t             mConcurrent;

  
  uint32_t             mServerPushedResources;

  
  uint32_t             mServerInitialWindow;

  
  
  
  
  
  uint32_t             mOutputQueueSize;
  uint32_t             mOutputQueueUsed;
  uint32_t             mOutputQueueSent;
  nsAutoArrayPtr<char> mOutputQueueBuffer;

  PRIntervalTime       mPingThreshold;
  PRIntervalTime       mLastReadEpoch;     
  PRIntervalTime       mLastDataReadEpoch; 
  PRIntervalTime       mPingSentEpoch;
  uint32_t             mNextPingID;

  PRIntervalTime       mPreviousPingThreshold; 
  bool                 mPreviousUsed;          

  
  nsDeque  mGoAwayStreamsToRestart;

  
  
  
  uint64_t        mSerial;

private:

  void DispatchOnTunnel(nsAHttpTransaction *, nsIInterfaceRequestor *);
  void RegisterTunnel(SpdyStream3 *);
  void UnRegisterTunnel(SpdyStream3 *);
  uint32_t FindTunnelCount(nsHttpConnectionInfo *);

  nsDataHashtable<nsCStringHashKey, uint32_t> mTunnelHash;
};

}} 

#endif 
