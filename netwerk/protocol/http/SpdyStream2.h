





#ifndef mozilla_net_SpdyStream2_h
#define mozilla_net_SpdyStream2_h

#include "nsAHttpTransaction.h"

namespace mozilla { namespace net {

class SpdyStream2 : public nsAHttpSegmentReader
                  , public nsAHttpSegmentWriter
{
public:
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  SpdyStream2(nsAHttpTransaction *,
             SpdySession2 *, nsISocketTransport *,
             PRUint32, z_stream *, PRInt32);

  PRUint32 StreamID() { return mStreamID; }

  nsresult ReadSegments(nsAHttpSegmentReader *,  PRUint32, PRUint32 *);
  nsresult WriteSegments(nsAHttpSegmentWriter *, PRUint32, PRUint32 *);

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

  nsAHttpTransaction *Transaction()
  {
    return mTransaction;
  }

  void Close(nsresult reason);

  void SetRecvdFin(bool aStatus) { mRecvdFin = aStatus ? 1 : 0; }
  bool RecvdFin() { return mRecvdFin; }

  void UpdateTransportSendEvents(PRUint32 count);
  void UpdateTransportReadEvents(PRUint32 count);

  
  
  static const char *kDictionary;
  static void *zlib_allocator(void *, uInt, uInt);
  static void zlib_destructor(void *, void *);

private:

  
  
  
  friend class nsAutoPtr<SpdyStream2>;
  ~SpdyStream2();

  enum stateType {
    GENERATING_SYN_STREAM,
    SENDING_SYN_STREAM,
    GENERATING_REQUEST_BODY,
    SENDING_REQUEST_BODY,
    SENDING_FIN_STREAM,
    UPSTREAM_COMPLETE
  };

  static PLDHashOperator hdrHashEnumerate(const nsACString &,
                                          nsAutoPtr<nsCString> &,
                                          void *);

  void     ChangeState(enum stateType);
  nsresult ParseHttpRequestHeaders(const char *, PRUint32, PRUint32 *);
  nsresult TransmitFrame(const char *, PRUint32 *);
  void     GenerateDataFrameHeader(PRUint32, bool);

  void     CompressToFrame(const nsACString &);
  void     CompressToFrame(const nsACString *);
  void     CompressToFrame(const char *, PRUint32);
  void     CompressToFrame(PRUint16);
  void     CompressFlushFrame();
  void     ExecuteCompress(PRUint32);
  
  
  
  
  enum stateType mUpstreamState;

  
  
  
  
  nsRefPtr<nsAHttpTransaction> mTransaction;

  
  SpdySession2                *mSession;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader        *mSegmentReader;
  nsAHttpSegmentWriter        *mSegmentWriter;

  
  PRUint32                    mStreamID;

  
  PRUint32                    mChunkSize;

  
  PRUint32                     mSynFrameComplete     : 1;

  
  
  PRUint32                     mRequestBlockedOnRead : 1;

  
  
  PRUint32                     mSentFinOnData        : 1;

  
  
  PRUint32                     mRecvdFin             : 1;

  
  PRUint32                     mFullyOpen            : 1;

  
  PRUint32                     mSentWaitingFor       : 1;

  
  
  nsAutoArrayPtr<char>         mTxInlineFrame;
  PRUint32                     mTxInlineFrameSize;
  PRUint32                     mTxInlineFrameUsed;

  
  
  
  PRUint32                     mTxStreamFrameSize;

  
  
  
  z_stream                     *mZlib;
  nsCString                    mFlatHttpRequestHeaders;

  
  
  
  
  
  PRInt64                      mRequestBodyLenRemaining;

  
  PRInt32                      mPriority;

  
  PRUint64                     mTotalSent;
  PRUint64                     mTotalRead;
};

}} 

#endif 
