





#ifndef mozilla_net_TLSFilterTransaction_h
#define mozilla_net_TLSFilterTransaction_h

#include "mozilla/Attributes.h"
#include "nsAHttpTransaction.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsISocketTransport.h"
#include "nsITimer.h"
#include "NullHttpTransaction.h"

































































struct PRSocketOptionData;

namespace mozilla { namespace net {

class nsHttpRequestHead;
class TLSFilterTransaction;

class NudgeTunnelCallback : public nsISupports
{
public:
  virtual void OnTunnelNudged(TLSFilterTransaction *) = 0;
};

#define NS_DECL_NUDGETUNNELCALLBACK void OnTunnelNudged(TLSFilterTransaction *);

class TLSFilterTransaction MOZ_FINAL
  : public nsAHttpTransaction
  , public nsAHttpSegmentReader
  , public nsAHttpSegmentWriter
  , public nsITimerCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION
  NS_DECL_NSAHTTPSEGMENTREADER
  NS_DECL_NSAHTTPSEGMENTWRITER
  NS_DECL_NSITIMERCALLBACK

  TLSFilterTransaction(nsAHttpTransaction *aWrappedTransaction,
                       const char *tlsHost, int32_t tlsPort,
                       nsAHttpSegmentReader *reader,
                       nsAHttpSegmentWriter *writer);
  ~TLSFilterTransaction();

  const nsAHttpTransaction *Transaction() const { return mTransaction.get(); }
  nsresult CommitToSegmentSize(uint32_t size, bool forceCommitment);
  nsresult GetTransactionSecurityInfo(nsISupports **);
  nsresult NudgeTunnel(NudgeTunnelCallback *callback);

private:
  nsresult StartTimerCallback();
  void Cleanup();
  int32_t FilterOutput(const char *aBuf, int32_t aAmount);
  int32_t FilterInput(char *aBuf, int32_t aAmount);

  static PRStatus GetPeerName(PRFileDesc *fd, PRNetAddr*addr);
  static PRStatus GetSocketOption(PRFileDesc *fd, PRSocketOptionData *data);
  static PRStatus SetSocketOption(PRFileDesc *fd, const PRSocketOptionData *data);
  static int32_t FilterWrite(PRFileDesc *fd, const void *buf, int32_t amount);
  static int32_t FilterRead(PRFileDesc *fd, void *buf, int32_t amount);
  static int32_t FilterSend(PRFileDesc *fd, const void *buf, int32_t amount, int flags,
                             PRIntervalTime timeout);
  static int32_t FilterRecv(PRFileDesc *fd, void *buf, int32_t amount, int flags,
                             PRIntervalTime timeout);
  static PRStatus FilterClose(PRFileDesc *fd);

private:
  nsRefPtr<nsAHttpTransaction> mTransaction;
  nsCOMPtr<nsISupports> mSecInfo;
  nsCOMPtr<nsITimer> mTimer;
  nsRefPtr<NudgeTunnelCallback> mNudgeCallback;

  
  nsAutoArrayPtr<char> mEncryptedText;
  uint32_t mEncryptedTextUsed;
  uint32_t mEncryptedTextSize;

  PRFileDesc *mFD;
  nsAHttpSegmentReader *mSegmentReader;
  nsAHttpSegmentWriter *mSegmentWriter;

  nsresult mFilterReadCode;
  bool mForce;
  bool mReadSegmentBlocked;
  uint32_t mNudgeCounter;
};

class SocketTransportShim;
class InputStreamShim;
class OutputStreamShim;
class nsHttpConnection;
class ASpdySession;

class SpdyConnectTransaction MOZ_FINAL : public NullHttpTransaction
{
public:
  SpdyConnectTransaction(nsHttpConnectionInfo *ci,
                         nsIInterfaceRequestor *callbacks,
                         uint32_t caps,
                         nsAHttpTransaction *trans,
                         nsAHttpConnection *session);
  ~SpdyConnectTransaction();

  SpdyConnectTransaction *QuerySpdyConnectTransaction() { return this; }

  void MapStreamToHttpConnection(nsISocketTransport *aTransport,
                                 nsHttpConnectionInfo *aConnInfo);

  nsresult ReadSegments(nsAHttpSegmentReader *reader,
                        uint32_t count, uint32_t *countRead) MOZ_OVERRIDE MOZ_FINAL;
  nsresult WriteSegments(nsAHttpSegmentWriter *writer,
                         uint32_t count, uint32_t *countWritten) MOZ_OVERRIDE MOZ_FINAL;
  nsHttpRequestHead *RequestHead() MOZ_OVERRIDE MOZ_FINAL;
  void Close(nsresult reason) MOZ_OVERRIDE MOZ_FINAL;

private:
  friend class InputStreamShim;
  friend class OutputStreamShim;

  nsresult Flush(uint32_t count, uint32_t *countRead);
  void CreateShimError(nsresult code);

  nsCString             mConnectString;
  uint32_t              mConnectStringOffset;
  nsHttpRequestHead     *mRequestHead;

  nsAHttpConnection    *mSession;
  nsAHttpSegmentReader *mSegmentReader;

  nsAutoArrayPtr<char> mInputData;
  uint32_t             mInputDataSize;
  uint32_t             mInputDataUsed;
  uint32_t             mInputDataOffset;

  nsAutoArrayPtr<char> mOutputData;
  uint32_t             mOutputDataSize;
  uint32_t             mOutputDataUsed;
  uint32_t             mOutputDataOffset;

  TimeStamp                      mTimestampSyn;
  nsRefPtr<nsHttpConnection>     mTunneledConn;
  nsRefPtr<nsHttpConnectionInfo> mConnInfo;
  nsRefPtr<SocketTransportShim>  mTunnelTransport;
  nsRefPtr<InputStreamShim>      mTunnelStreamIn;
  nsRefPtr<OutputStreamShim>     mTunnelStreamOut;
};

}} 

#endif 
