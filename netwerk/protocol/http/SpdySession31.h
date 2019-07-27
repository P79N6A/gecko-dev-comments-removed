




#ifndef mozilla_net_SpdySession31_h
#define mozilla_net_SpdySession31_h



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

class SpdyPushedStream31;
class SpdyStream31;
class nsHttpTransaction;

class SpdySession31 MOZ_FINAL : public ASpdySession
                              , public nsAHttpConnection
                              , public nsAHttpSegmentReader
                              , public nsAHttpSegmentWriter
{
  ~SpdySession31();

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION
  NS_DECL_NSAHTTPCONNECTION(mConnection)
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  explicit SpdySession31(nsISocketTransport *);

  bool AddStream(nsAHttpTransaction *, int32_t,
                 bool, nsIInterfaceRequestor *);
  bool CanReuse() { return !mShouldGoAway && !mClosed; }
  bool RoomForMoreStreams();

  
  
  
  
  uint32_t  ReadTimeoutTick(PRIntervalTime now);

  
  PRIntervalTime IdleTime();

  
  uint32_t RegisterStreamID(SpdyStream31 *, uint32_t aNewID = 0);

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

  
  const static uint32_t kDefaultRwin = 64 * 1024;

  static nsresult HandleSynStream(SpdySession31 *);
  static nsresult HandleSynReply(SpdySession31 *);
  static nsresult HandleRstStream(SpdySession31 *);
  static nsresult HandleSettings(SpdySession31 *);
  static nsresult HandleNoop(SpdySession31 *);
  static nsresult HandlePing(SpdySession31 *);
  static nsresult HandleGoAway(SpdySession31 *);
  static nsresult HandleHeaders(SpdySession31 *);
  static nsresult HandleWindowUpdate(SpdySession31 *);
  static nsresult HandleCredential(SpdySession31 *);

  
  static void LogIO(SpdySession31 *, SpdyStream31 *, const char *,
                    const char *, uint32_t);

  
  void TransactionHasDataToWrite(nsAHttpTransaction *);

  
  void TransactionHasDataToWrite(SpdyStream31 *);

  
  virtual nsresult CommitToSegmentSize(uint32_t size, bool forceCommitment);
  nsresult BufferOutput(const char *, uint32_t, uint32_t *);
  void     FlushOutputQueue();
  uint32_t AmountOfOutputBuffered() { return mOutputQueueUsed - mOutputQueueSent; }

  uint32_t GetServerInitialStreamWindow() { return mServerInitialStreamWindow; }

  void ConnectPushedStream(SpdyStream31 *stream);
  void DecrementConcurrent(SpdyStream31 *stream);

  uint64_t Serial() { return mSerial; }

  void     PrintDiagnostics (nsCString &log);

  
  uint32_t SendingChunkSize() { return mSendingChunkSize; }
  uint32_t PushAllowance() { return mPushAllowance; }
  z_stream *UpstreamZlib() { return &mUpstreamZlib; }
  nsISocketTransport *SocketTransport() { return mSocketTransport; }
  int64_t RemoteSessionWindow() { return mRemoteSessionWindow; }
  void DecrementRemoteSessionWindow (uint32_t bytes) { mRemoteSessionWindow -= bytes; }

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
  void        CleanupStream(SpdyStream31 *, nsresult, rstReason);
  void        CloseStream(SpdyStream31 *, nsresult);
  void        GenerateSettings();
  void        RemoveStreamFromQueues(SpdyStream31 *);

  void        SetWriteCallbacks();
  void        RealignOutputQueue();

  bool        RoomForMoreConcurrent();
  void        ActivateStream(SpdyStream31 *);
  void        ProcessPending();
  nsresult    SetInputFrameDataStream(uint32_t);
  bool        VerifyStream(SpdyStream31 *, uint32_t);
  void        SetNeedsCleanup();

  void        UpdateLocalRwin(SpdyStream31 *stream, uint32_t bytes);
  void        UpdateLocalStreamWindow(SpdyStream31 *stream, uint32_t bytes);
  void        UpdateLocalSessionWindow(uint32_t bytes);

  
  
  nsresult   NetworkRead(nsAHttpSegmentWriter *, char *, uint32_t, uint32_t *);

  static PLDHashOperator ShutdownEnumerator(nsAHttpTransaction *,
                                            nsAutoPtr<SpdyStream31> &,
                                            void *);

  static PLDHashOperator GoAwayEnumerator(nsAHttpTransaction *,
                                          nsAutoPtr<SpdyStream31> &,
                                          void *);

  static PLDHashOperator UpdateServerRwinEnumerator(nsAHttpTransaction *,
                                                    nsAutoPtr<SpdyStream31> &,
                                                    void *);

  static PLDHashOperator RestartBlockedOnRwinEnumerator(nsAHttpTransaction *,
                                                        nsAutoPtr<SpdyStream31> &,
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

  
  
  
  
  
  
  
  nsDataHashtable<nsUint32HashKey, SpdyStream31 *>     mStreamIDHash;
  nsClassHashtable<nsPtrHashKey<nsAHttpTransaction>,
    SpdyStream31>                                     mStreamTransactionHash;

  nsDeque                                             mReadyForWrite;
  nsDeque                                             mQueuedStreams;
  nsDeque                                             mReadyForRead;
  nsTArray<SpdyPushedStream31 *>                      mPushedStreams;

  
  
  
  
  
  z_stream            mDownstreamZlib;
  z_stream            mUpstreamZlib;

  
  
  uint32_t             mInputFrameBufferSize;
  uint32_t             mInputFrameBufferUsed;
  nsAutoArrayPtr<char> mInputFrameBuffer;

  
  
  
  uint32_t             mInputFrameDataSize;
  uint32_t             mInputFrameDataRead;
  bool                 mInputFrameDataLast; 

  
  
  
  SpdyStream31          *mInputFrameDataStream;

  
  
  
  
  
  SpdyStream31          *mNeedsCleanup;

  
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

  
  uint32_t             mServerInitialStreamWindow;

  
  
  
  int64_t              mLocalSessionWindow;

  
  
  
  int64_t              mRemoteSessionWindow;

  
  
  
  
  
  uint32_t             mOutputQueueSize;
  uint32_t             mOutputQueueUsed;
  uint32_t             mOutputQueueSent;
  nsAutoArrayPtr<char> mOutputQueueBuffer;

  PRIntervalTime       mPingThreshold;
  PRIntervalTime       mLastReadEpoch;     
  PRIntervalTime       mLastDataReadEpoch; 
  PRIntervalTime       mPingSentEpoch;
  uint32_t             mNextPingID;

  
  nsDeque  mGoAwayStreamsToRestart;

  
  
  
  uint64_t        mSerial;

private:

  void DispatchOnTunnel(nsAHttpTransaction *, nsIInterfaceRequestor *);
  void RegisterTunnel(SpdyStream31 *);
  void UnRegisterTunnel(SpdyStream31 *);
  uint32_t FindTunnelCount(nsHttpConnectionInfo *);

  nsDataHashtable<nsCStringHashKey, uint32_t> mTunnelHash;
};

}} 

#endif 
