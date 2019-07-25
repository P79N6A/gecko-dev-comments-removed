






































#ifndef mozilla_net_SpdyStream_h
#define mozilla_net_SpdyStream_h

#include "nsAHttpTransaction.h"

namespace mozilla { namespace net {

class SpdyStream : public nsAHttpSegmentReader
                 , public nsAHttpSegmentWriter
{
public:
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER

  SpdyStream(nsAHttpTransaction *,
             SpdySession *, nsISocketTransport *,
             PRUint32, z_stream *, PRInt32);
  ~SpdyStream();

  PRUint32 StreamID() { return mStreamID; }

  nsresult ReadSegments(nsAHttpSegmentReader *,  PRUint32, PRUint32 *);
  nsresult WriteSegments(nsAHttpSegmentWriter *, PRUint32, PRUint32 *);

  bool BlockedOnWrite()
  {
    return static_cast<bool>(mBlockedOnWrite);
  }

  bool RequestBlockedOnRead()
  {
    return static_cast<bool>(mRequestBlockedOnRead);
  }

  
  bool SetFullyOpen()
  {
    bool result = !mFullyOpen;
    mFullyOpen = 1;
    return result;
  }

  nsAHttpTransaction *Transaction()
  {
    return mTransaction;
  }

  void Close(nsresult reason);

  void SetRecvdFin(bool aStatus) { mRecvdFin = aStatus ? 1 : 0; }
  bool RecvdFin() { return mRecvdFin; }

  
  
  static const char *kDictionary;
  static void *zlib_allocator(void *, uInt, uInt);
  static void zlib_destructor(void *, void *);

private:

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

  void     ChangeState(enum stateType );
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

  
  SpdySession                *mSession;

  
  nsISocketTransport         *mSocketTransport;

  
  
  
  nsAHttpSegmentReader        *mSegmentReader;
  nsAHttpSegmentWriter        *mSegmentWriter;

  
  PRUint32                    mStreamID;

  
  PRUint32                    mChunkSize;

  
  PRUint32                     mSynFrameComplete     : 1;

  
  
  
  PRUint32                     mBlockedOnWrite       : 1;

  
  
  PRUint32                     mRequestBlockedOnRead : 1;

  
  
  PRUint32                     mSentFinOnData        : 1;

  
  
  PRUint32                     mRecvdFin             : 1;

  
  PRUint32                     mFullyOpen            : 1;

  
  
  nsAutoArrayPtr<char>         mTxInlineFrame;
  PRUint32                     mTxInlineFrameAllocation;
  PRUint32                     mTxInlineFrameSize;
  PRUint32                     mTxInlineFrameSent;

  
  
  
  PRUint32                     mTxStreamFrameSize;
  PRUint32                     mTxStreamFrameSent;

  
  
  
  z_stream                     *mZlib;
  nsCString                    mFlatHttpRequestHeaders;

  
  
  
  
  
  PRInt64                      mRequestBodyLen;

  
  PRInt32                      mPriority;

};

}} 

#endif 
