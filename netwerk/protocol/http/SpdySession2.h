





#ifndef mozilla_net_SpdySession2_h
#define mozilla_net_SpdySession2_h




#include "ASpdySession.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsDeque.h"
#include "nsHashKeys.h"
#include "zlib.h"
#include "mozilla/Attributes.h"

class nsHttpConnection;
class nsISocketTransport;

namespace mozilla { namespace net {

class SpdyStream2;

class SpdySession2 MOZ_FINAL : public ASpdySession
                             , public nsAHttpConnection
                             , public nsAHttpSegmentReader
                             , public nsAHttpSegmentWriter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION
  NS_DECL_NSAHTTPCONNECTION(mConnection)
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  SpdySession2(nsAHttpTransaction *, nsISocketTransport *, int32_t);
  ~SpdySession2();

  bool AddStream(nsAHttpTransaction *, int32_t);
  bool CanReuse() { return !mShouldGoAway && !mClosed; }
  bool RoomForMoreStreams();

  
  void ReadTimeoutTick(PRIntervalTime now);
  
  
  PRIntervalTime IdleTime();

  uint32_t RegisterStreamID(SpdyStream2 *);

  const static uint8_t kFlag_Control   = 0x80;

  const static uint8_t kFlag_Data_FIN  = 0x01;
  const static uint8_t kFlag_Data_UNI  = 0x02;
  const static uint8_t kFlag_Data_ZLIB = 0x02;
  
  
  
  
  
  
  
  
  

  const static uint8_t kPri00   = 0 << 6; 
  const static uint8_t kPri01   = 1 << 6;
  const static uint8_t kPri02   = 2 << 6;
  const static uint8_t kPri03   = 3 << 6; 

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
    CONTROL_TYPE_LAST = 10
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
    RST_BAD_ASSOC_STREAM = 8
  };

  enum
  {
    SETTINGS_TYPE_UPLOAD_BW = 1, 
    SETTINGS_TYPE_DOWNLOAD_BW = 2, 
    SETTINGS_TYPE_RTT = 3, 
    SETTINGS_TYPE_MAX_CONCURRENT = 4, 
    SETTINGS_TYPE_CWND = 5, 
    SETTINGS_TYPE_DOWNLOAD_RETRANS_RATE = 6, 
    SETTINGS_TYPE_INITIAL_WINDOW = 7  
  };

  
  
  
  
  const static uint32_t kDefaultBufferSize = 2048;

  
  const static uint32_t kDefaultQueueSize =  32768;
  const static uint32_t kQueueMinimumCleanup = 24576;
  const static uint32_t kQueueTailRoom    =  4096;
  const static uint32_t kQueueReserved    =  1024;

  const static uint32_t kDefaultMaxConcurrent = 100;
  const static uint32_t kMaxStreamID = 0x7800000;
  
  
  
  const static uint32_t kDeadStreamID = 0xffffdead;
  
  static nsresult HandleSynStream(SpdySession2 *);
  static nsresult HandleSynReply(SpdySession2 *);
  static nsresult HandleRstStream(SpdySession2 *);
  static nsresult HandleSettings(SpdySession2 *);
  static nsresult HandleNoop(SpdySession2 *);
  static nsresult HandlePing(SpdySession2 *);
  static nsresult HandleGoAway(SpdySession2 *);
  static nsresult HandleHeaders(SpdySession2 *);
  static nsresult HandleWindowUpdate(SpdySession2 *);

  static void EnsureBuffer(nsAutoArrayPtr<char> &,
                           uint32_t, uint32_t, uint32_t &);

  
  static void LogIO(SpdySession2 *, SpdyStream2 *, const char *,
                    const char *, uint32_t);

  
  void TransactionHasDataToWrite(nsAHttpTransaction *);

  
  void TransactionHasDataToWrite(SpdyStream2 *);

  
  virtual nsresult CommitToSegmentSize(uint32_t size, bool forceCommitment);
  
  void     PrintDiagnostics (nsCString &log);

private:

  enum stateType {
    BUFFERING_FRAME_HEADER,
    BUFFERING_CONTROL_FRAME,
    PROCESSING_DATA_FRAME,
    DISCARDING_DATA_FRAME,
    PROCESSING_CONTROL_SYN_REPLY,
    PROCESSING_CONTROL_RST_STREAM
  };

  nsresult    HandleSynReplyForValidStream();
  uint32_t    GetWriteQueueSize();
  void        ChangeDownstreamState(enum stateType);
  void        ResetDownstreamState();
  nsresult    DownstreamUncompress(char *, uint32_t);
  void        zlibInit();
  nsresult    FindHeader(nsCString, nsDependentCSubstring &);
  nsresult    ConvertHeaders(nsDependentCSubstring &,
                             nsDependentCSubstring &);
  void        GeneratePing(uint32_t);
  void        GenerateRstStream(uint32_t, uint32_t);
  void        GenerateGoAway();
  void        CleanupStream(SpdyStream2 *, nsresult, rstReason);
  void        CloseStream(SpdyStream2 *, nsresult);

  void        SetWriteCallbacks();
  void        FlushOutputQueue();
  void        RealignOutputQueue();

  bool        RoomForMoreConcurrent();
  void        ActivateStream(SpdyStream2 *);
  void        ProcessPending();
  nsresult    SetInputFrameDataStream(uint32_t);
  bool        VerifyStream(SpdyStream2 *, uint32_t);
  void        SetNeedsCleanup();

  
  
  nsresult   NetworkRead(nsAHttpSegmentWriter *, char *, uint32_t, uint32_t *);
  
  static PLDHashOperator ShutdownEnumerator(nsAHttpTransaction *,
                                            nsAutoPtr<SpdyStream2> &,
                                            void *);

  static PLDHashOperator GoAwayEnumerator(nsAHttpTransaction *,
                                          nsAutoPtr<SpdyStream2> &,
                                          void *);

  
  
  
  nsRefPtr<nsAHttpConnection> mConnection;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader       *mSegmentReader;
  nsAHttpSegmentWriter       *mSegmentWriter;

  uint32_t          mSendingChunkSize;        
  uint32_t          mNextStreamID;            
  uint32_t          mConcurrentHighWater;     

  stateType         mDownstreamState; 

  
  
  
  
  
  
  
  nsDataHashtable<nsUint32HashKey, SpdyStream2 *>     mStreamIDHash;
  nsClassHashtable<nsPtrHashKey<nsAHttpTransaction>,
                   SpdyStream2>                       mStreamTransactionHash;
  nsDeque                                             mReadyForWrite;
  nsDeque                                             mQueuedStreams;

  
  
  
  
  nsDeque           mUrgentForWrite;

  
  
  
  z_stream            mDownstreamZlib;
  z_stream            mUpstreamZlib;

  
  
  uint32_t             mInputFrameBufferSize;
  uint32_t             mInputFrameBufferUsed;
  nsAutoArrayPtr<char> mInputFrameBuffer;
  
  
  
  
  uint32_t             mInputFrameDataSize;
  uint32_t             mInputFrameDataRead;
  bool                 mInputFrameDataLast; 

  
  
  
  SpdyStream2          *mInputFrameDataStream;
  
  
  
  
  
  
  SpdyStream2          *mNeedsCleanup;

  
  uint32_t             mFrameControlType;

  
  uint32_t             mDownstreamRstReason;

  
  
  
  
  uint32_t             mDecompressBufferSize;
  uint32_t             mDecompressBufferUsed;
  nsAutoArrayPtr<char> mDecompressBuffer;

  
  nsCString            mFlatHTTPResponseHeaders;
  uint32_t             mFlatHTTPResponseHeadersOut;

  
  
  
  
  
  bool                 mShouldGoAway;

  
  bool                 mClosed;

  
  bool                 mCleanShutdown;

  
  
  uint32_t             mGoAwayID;

  
  
  
  uint32_t             mMaxConcurrent;

  
  
  
  uint32_t             mConcurrent;

  
  uint32_t             mServerPushedResources;

  
  
  
  
  
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
};

}} 

#endif 
