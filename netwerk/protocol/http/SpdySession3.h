




#ifndef mozilla_net_SpdySession3_h
#define mozilla_net_SpdySession3_h




#include "ASpdySession.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsDeque.h"
#include "nsHashKeys.h"
#include "zlib.h"

class nsHttpConnection;
class nsISocketTransport;

namespace mozilla { namespace net {

class SpdyStream3;

class SpdySession3 : public ASpdySession
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

  SpdySession3(nsAHttpTransaction *, nsISocketTransport *, PRInt32);
  ~SpdySession3();

  bool AddStream(nsAHttpTransaction *, PRInt32);
  bool CanReuse() { return !mShouldGoAway && !mClosed; }
  bool RoomForMoreStreams();

  
  void ReadTimeoutTick(PRIntervalTime now);
  
  
  PRIntervalTime IdleTime();

  PRUint32 RegisterStreamID(SpdyStream3 *);

  const static PRUint8 kVersion        = 3;

  const static PRUint8 kFlag_Control   = 0x80;

  const static PRUint8 kFlag_Data_FIN  = 0x01;
  const static PRUint8 kFlag_Data_UNI  = 0x02;
  
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

  
  
  
  
  const static PRUint32 kDefaultBufferSize = 2048;

  
  const static PRUint32 kDefaultQueueSize =  16384;
  const static PRUint32 kQueueMinimumCleanup = 8192;
  const static PRUint32 kQueueTailRoom    =  4096;
  const static PRUint32 kQueueReserved    =  1024;

  const static PRUint32 kDefaultMaxConcurrent = 100;
  const static PRUint32 kMaxStreamID = 0x7800000;

  
  
  const static PRUint32 kDeadStreamID = 0xffffdead;

  
  
  
  
  const static PRUint32 kInitialRwin = 256 * 1024 * 1024;
  const static PRUint32 kMinimumToAck = 64 * 1024;

  
  const static PRUint32 kDefaultServerRwin = 64 * 1024;

  static nsresult HandleSynStream(SpdySession3 *);
  static nsresult HandleSynReply(SpdySession3 *);
  static nsresult HandleRstStream(SpdySession3 *);
  static nsresult HandleSettings(SpdySession3 *);
  static nsresult HandleNoop(SpdySession3 *);
  static nsresult HandlePing(SpdySession3 *);
  static nsresult HandleGoAway(SpdySession3 *);
  static nsresult HandleHeaders(SpdySession3 *);
  static nsresult HandleWindowUpdate(SpdySession3 *);

  static void EnsureBuffer(nsAutoArrayPtr<char> &,
                           PRUint32, PRUint32, PRUint32 &);

  
  static void LogIO(SpdySession3 *, SpdyStream3 *, const char *,
                    const char *, PRUint32);

  
  void TransactionHasDataToWrite(nsAHttpTransaction *);

  
  void TransactionHasDataToWrite(SpdyStream3 *);

  
  virtual nsresult CommitToSegmentSize(PRUint32 size);
  
  PRUint32 GetServerInitialWindow() { return mServerInitialWindow; }

private:

  enum stateType {
    BUFFERING_FRAME_HEADER,
    BUFFERING_CONTROL_FRAME,
    PROCESSING_DATA_FRAME,
    DISCARDING_DATA_FRAME,
    PROCESSING_COMPLETE_HEADERS,
    PROCESSING_CONTROL_RST_STREAM
  };

  void        DeterminePingThreshold();
  nsresult    ResponseHeadersComplete();
  PRUint32    GetWriteQueueSize();
  void        ChangeDownstreamState(enum stateType);
  void        ResetDownstreamState();
  nsresult    UncompressAndDiscard(PRUint32, PRUint32);
  void        zlibInit();
  void        GeneratePing(PRUint32);
  void        ClearPing(bool);
  void        GenerateRstStream(PRUint32, PRUint32);
  void        GenerateGoAway();
  void        CleanupStream(SpdyStream3 *, nsresult, rstReason);
  void        GenerateSettings();

  void        SetWriteCallbacks();
  void        FlushOutputQueue();

  bool        RoomForMoreConcurrent();
  void        ActivateStream(SpdyStream3 *);
  void        ProcessPending();
  nsresult    SetInputFrameDataStream(PRUint32);
  bool        VerifyStream(SpdyStream3 *, PRUint32);
  void        SetNeedsCleanup();

  void        UpdateLocalRwin(SpdyStream3 *stream, PRUint32 bytes);

  
  
  nsresult   NetworkRead(nsAHttpSegmentWriter *, char *, PRUint32, PRUint32 *);
  
  static PLDHashOperator ShutdownEnumerator(nsAHttpTransaction *,
                                            nsAutoPtr<SpdyStream3> &,
                                            void *);

  static PLDHashOperator UpdateServerRwinEnumerator(nsAHttpTransaction *,
                                                    nsAutoPtr<SpdyStream3> &,
                                                    void *);

  
  
  
  nsRefPtr<nsAHttpConnection> mConnection;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader       *mSegmentReader;
  nsAHttpSegmentWriter       *mSegmentWriter;

  PRUint32          mSendingChunkSize;        
  PRUint32          mNextStreamID;            
  PRUint32          mConcurrentHighWater;     

  stateType         mDownstreamState; 

  
  
  
  
  
  
  nsDataHashtable<nsUint32HashKey, SpdyStream3 *>     mStreamIDHash;
  nsClassHashtable<nsPtrHashKey<nsAHttpTransaction>,
                   SpdyStream3>                       mStreamTransactionHash;
  nsDeque                                             mReadyForWrite;
  nsDeque                                             mQueuedStreams;

  
  
  
  
  
  z_stream            mDownstreamZlib;
  z_stream            mUpstreamZlib;

  
  
  PRUint32             mInputFrameBufferSize;
  PRUint32             mInputFrameBufferUsed;
  nsAutoArrayPtr<char> mInputFrameBuffer;
  
  
  
  
  PRUint32             mInputFrameDataSize;
  PRUint32             mInputFrameDataRead;
  bool                 mInputFrameDataLast; 

  
  
  
  SpdyStream3          *mInputFrameDataStream;
  
  
  
  
  
  
  SpdyStream3          *mNeedsCleanup;

  
  PRUint32             mFrameControlType;

  
  PRUint32             mDownstreamRstReason;

  
  
  nsCString            mFlatHTTPResponseHeaders;
  PRUint32             mFlatHTTPResponseHeadersOut;

  
  
  
  
  
  bool                 mShouldGoAway;

  
  bool                 mClosed;

  
  bool                 mCleanShutdown;

  
  
  
  bool                 mDataPending;

  
  
  PRUint32             mGoAwayID;

  
  
  
  PRUint32             mMaxConcurrent;

  
  
  
  PRUint32             mConcurrent;

  
  PRUint32             mServerPushedResources;

  
  PRUint32             mServerInitialWindow;

  
  
  
  
  
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
