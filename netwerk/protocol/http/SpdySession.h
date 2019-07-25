






































#ifndef mozilla_net_SpdySession_h
#define mozilla_net_SpdySession_h




#include "nsAHttpTransaction.h"
#include "nsAHttpConnection.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsDeque.h"
#include "nsHashKeys.h"
#include "zlib.h"

class nsHttpConnection;
class nsISocketTransport;

namespace mozilla { namespace net {

class SpdyStream;

class SpdySession : public nsAHttpTransaction
                  , public nsAHttpConnection
                  , public nsAHttpSegmentReader
                  , public nsAHttpSegmentWriter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION
  NS_DECL_NSAHTTPCONNECTION
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  SpdySession(nsAHttpTransaction *, nsISocketTransport *, PRInt32);
  ~SpdySession();

  bool AddStream(nsAHttpTransaction *, PRInt32);
  bool CanReuse() { return !mShouldGoAway && !mClosed; }
  void DontReuse();
  bool RoomForMoreStreams();
  PRUint32 RegisterStreamID(SpdyStream *);

  const static PRUint8 kFlag_Control   = 0x80;

  const static PRUint8 kFlag_Data_FIN  = 0x01;
  const static PRUint8 kFlag_Data_UNI  = 0x02;
  const static PRUint8 kFlag_Data_ZLIB = 0x02;
  
  const static PRUint8 kPri00   = 0x00;
  const static PRUint8 kPri01   = 0x40;
  const static PRUint8 kPri02   = 0x80;
  const static PRUint8 kPri03   = 0xC0;

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

  enum
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

  
  
  
  
  const static PRUint32 kDefaultBufferSize = 2000;

  const static PRUint32 kDefaultQueueSize =  16000;
  const static PRUint32 kQueueTailRoom    =  4000;
  const static PRUint32 kSendingChunkSize = 4000;
  const static PRUint32 kDefaultMaxConcurrent = 100;
  const static PRUint32 kMaxStreamID = 0x7800000;
  
  static nsresult HandleSynStream(SpdySession *);
  static nsresult HandleSynReply(SpdySession *);
  static nsresult HandleRstStream(SpdySession *);
  static nsresult HandleSettings(SpdySession *);
  static nsresult HandleNoop(SpdySession *);
  static nsresult HandlePing(SpdySession *);
  static nsresult HandleGoAway(SpdySession *);
  static nsresult HandleHeaders(SpdySession *);
  static nsresult HandleWindowUpdate(SpdySession *);

  static void EnsureBuffer(nsAutoArrayPtr<char> &,
                           PRUint32, PRUint32, PRUint32 &);
private:

  enum stateType {
    BUFFERING_FRAME_HEADER,
    BUFFERING_CONTROL_FRAME,
    PROCESSING_DATA_FRAME,
    DISCARD_DATA_FRAME,
    PROCESSING_CONTROL_SYN_REPLY,
    PROCESSING_CONTROL_RST_STREAM
  };

  PRUint32    WriteQueueSize();
  void        ChangeDownstreamState(enum stateType);
  nsresult    DownstreamUncompress(char *, PRUint32);
  void        zlibInit();
  nsresult    FindHeader(nsCString, nsDependentCSubstring &);
  nsresult    ConvertHeaders(nsDependentCSubstring &,
                             nsDependentCSubstring &);
  void        GeneratePing(PRUint32);
  void        GenerateRstStream(PRUint32, PRUint32);
  void        GenerateGoAway();
  void        CleanupStream(SpdyStream *, nsresult);

  void        SetWriteCallbacks(nsAHttpTransaction *);
  void        FlushOutputQueue();

  bool        RoomForMoreConcurrent();
  void        ActivateStream(SpdyStream *);
  void        ProcessPending();

  static PLDHashOperator Shutdown(nsAHttpTransaction *,
                                  nsAutoPtr<SpdyStream> &,
                                  void *);

  
  
  
  nsRefPtr<nsAHttpConnection> mConnection;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader       *mSegmentReader;
  nsAHttpSegmentWriter       *mSegmentWriter;

  PRUint32          mSendingChunkSize;        
  PRUint32          mNextStreamID;            
  PRUint32          mConcurrentHighWater;     

  stateType         mDownstreamState; 

  
  
  
  
  
  
  
  nsDataHashtable<nsUint32HashKey, SpdyStream *>      mStreamIDHash;
  nsClassHashtable<nsPtrHashKey<nsAHttpTransaction>,
                   SpdyStream>                        mStreamTransactionHash;
  nsDeque                                             mReadyForWrite;
  nsDeque                                             mQueuedStreams;

  
  
  
  
  nsDeque           mUrgentForWrite;

  
  
  
  SpdyStream        *mPartialFrame;

  
  
  
  z_stream            mDownstreamZlib;
  z_stream            mUpstreamZlib;

  
  
  PRUint32             mFrameBufferSize;
  PRUint32             mFrameBufferUsed;
  nsAutoArrayPtr<char> mFrameBuffer;
  
  
  
  
  PRUint32             mFrameDataSize;
  PRUint32             mFrameDataRead;
  bool                 mFrameDataLast; 

  
  
  
  SpdyStream          *mFrameDataStream;
  
  
  SpdyStream          *mNeedsCleanup;

  
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
};

}} 

#endif 
