





#ifndef mozilla_net_SpdySession2_h
#define mozilla_net_SpdySession2_h




#include "ASpdySession.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsDeque.h"
#include "nsHashKeys.h"
#include "zlib.h"

class nsHttpConnection;
class nsISocketTransport;

namespace mozilla { namespace net {

class SpdyStream2;

class SpdySession2 : public ASpdySession
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

  SpdySession2(nsAHttpTransaction *, nsISocketTransport *, PRInt32);
  ~SpdySession2();

  bool AddStream(nsAHttpTransaction *, PRInt32);
  bool CanReuse() { return !mShouldGoAway && !mClosed; }
  bool RoomForMoreStreams();

  
  void ReadTimeoutTick(PRIntervalTime now);
  
  
  PRIntervalTime IdleTime();

  PRUint32 RegisterStreamID(SpdyStream2 *);

  const static PRUint8 kFlag_Control   = 0x80;

  const static PRUint8 kFlag_Data_FIN  = 0x01;
  const static PRUint8 kFlag_Data_UNI  = 0x02;
  const static PRUint8 kFlag_Data_ZLIB = 0x02;
  
  
  
  
  
  
  
  
  

  const static PRUint8 kPri00   = 0 << 6; 
  const static PRUint8 kPri01   = 1 << 6;
  const static PRUint8 kPri02   = 2 << 6;
  const static PRUint8 kPri03   = 3 << 6; 

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

  
  
  
  
  const static PRUint32 kDefaultBufferSize = 2048;

  
  const static PRUint32 kDefaultQueueSize =  16384;
  const static PRUint32 kQueueMinimumCleanup = 8192;
  const static PRUint32 kQueueTailRoom    =  4096;
  const static PRUint32 kQueueReserved    =  1024;

  const static PRUint32 kDefaultMaxConcurrent = 100;
  const static PRUint32 kMaxStreamID = 0x7800000;
  
  
  
  const static PRUint32 kDeadStreamID = 0xffffdead;
  
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
                           PRUint32, PRUint32, PRUint32 &);

  
  static void LogIO(SpdySession2 *, SpdyStream2 *, const char *,
                    const char *, PRUint32);

  
  void TransactionHasDataToWrite(nsAHttpTransaction *);

  
  void TransactionHasDataToWrite(SpdyStream2 *);

  
  virtual nsresult CommitToSegmentSize(PRUint32 size);
  
private:

  enum stateType {
    BUFFERING_FRAME_HEADER,
    BUFFERING_CONTROL_FRAME,
    PROCESSING_DATA_FRAME,
    DISCARDING_DATA_FRAME,
    PROCESSING_CONTROL_SYN_REPLY,
    PROCESSING_CONTROL_RST_STREAM
  };

  void        DeterminePingThreshold();
  nsresult    HandleSynReplyForValidStream();
  PRUint32    GetWriteQueueSize();
  void        ChangeDownstreamState(enum stateType);
  void        ResetDownstreamState();
  nsresult    DownstreamUncompress(char *, PRUint32);
  void        zlibInit();
  nsresult    FindHeader(nsCString, nsDependentCSubstring &);
  nsresult    ConvertHeaders(nsDependentCSubstring &,
                             nsDependentCSubstring &);
  void        GeneratePing(PRUint32);
  void        ClearPing(bool);
  void        GenerateRstStream(PRUint32, PRUint32);
  void        GenerateGoAway();
  void        CleanupStream(SpdyStream2 *, nsresult, rstReason);

  void        SetWriteCallbacks();
  void        FlushOutputQueue();

  bool        RoomForMoreConcurrent();
  void        ActivateStream(SpdyStream2 *);
  void        ProcessPending();
  nsresult    SetInputFrameDataStream(PRUint32);
  bool        VerifyStream(SpdyStream2 *, PRUint32);
  void        SetNeedsCleanup();

  
  
  nsresult   NetworkRead(nsAHttpSegmentWriter *, char *, PRUint32, PRUint32 *);
  
  static PLDHashOperator ShutdownEnumerator(nsAHttpTransaction *,
                                            nsAutoPtr<SpdyStream2> &,
                                            void *);

  
  
  
  nsRefPtr<nsAHttpConnection> mConnection;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader       *mSegmentReader;
  nsAHttpSegmentWriter       *mSegmentWriter;

  PRUint32          mSendingChunkSize;        
  PRUint32          mNextStreamID;            
  PRUint32          mConcurrentHighWater;     

  stateType         mDownstreamState; 

  
  
  
  
  
  
  
  nsDataHashtable<nsUint32HashKey, SpdyStream2 *>     mStreamIDHash;
  nsClassHashtable<nsPtrHashKey<nsAHttpTransaction>,
                   SpdyStream2>                       mStreamTransactionHash;
  nsDeque                                             mReadyForWrite;
  nsDeque                                             mQueuedStreams;

  
  
  
  
  nsDeque           mUrgentForWrite;

  
  
  
  z_stream            mDownstreamZlib;
  z_stream            mUpstreamZlib;

  
  
  PRUint32             mInputFrameBufferSize;
  PRUint32             mInputFrameBufferUsed;
  nsAutoArrayPtr<char> mInputFrameBuffer;
  
  
  
  
  PRUint32             mInputFrameDataSize;
  PRUint32             mInputFrameDataRead;
  bool                 mInputFrameDataLast; 

  
  
  
  SpdyStream2          *mInputFrameDataStream;
  
  
  
  
  
  
  SpdyStream2          *mNeedsCleanup;

  
  PRUint32             mFrameControlType;

  
  PRUint32             mDownstreamRstReason;

  
  
  
  
  PRUint32             mDecompressBufferSize;
  PRUint32             mDecompressBufferUsed;
  nsAutoArrayPtr<char> mDecompressBuffer;

  
  nsCString            mFlatHTTPResponseHeaders;
  PRUint32             mFlatHTTPResponseHeadersOut;

  
  
  
  
  
  bool                 mShouldGoAway;

  
  bool                 mClosed;

  
  bool                 mCleanShutdown;

  
  
  PRUint32             mGoAwayID;

  
  
  
  PRUint32             mMaxConcurrent;

  
  
  
  PRUint32             mConcurrent;

  
  PRUint32             mServerPushedResources;

  
  
  
  
  
  PRUint32             mOutputQueueSize;
  PRUint32             mOutputQueueUsed;
  PRUint32             mOutputQueueSent;
  nsAutoArrayPtr<char> mOutputQueueBuffer;

  PRIntervalTime       mPingThreshold;
  PRIntervalTime       mLastReadEpoch;     
  PRIntervalTime       mLastDataReadEpoch; 
  PRIntervalTime       mPingSentEpoch;
  PRUint32             mNextPingID;
  bool                 mPingThresholdExperiment;
};

}} 

#endif 
