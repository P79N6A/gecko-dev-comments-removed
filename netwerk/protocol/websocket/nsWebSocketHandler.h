







































#include "nsIWebSocketProtocol.h"
#include "nsIURI.h"
#include "nsISupports.h"
#include "nsIInterfaceRequestor.h"
#include "nsIEventTarget.h"
#include "nsIStreamListener.h"
#include "nsIProtocolHandler.h"
#include "nsISocketTransport.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsILoadGroup.h"
#include "nsITimer.h"
#include "nsIDNSListener.h"
#include "nsIHttpChannel.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIStringStream.h"
#include "nsIHttpChannelInternal.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsDeque.h"

namespace mozilla { namespace net {

class nsPostMessage;
class nsWSAdmissionManager;
class nsWSCompression;
class WSCallOnInputStreamReady;

class nsWebSocketHandler : public nsIWebSocketProtocol,
                           public nsIHttpUpgradeListener,
                           public nsIStreamListener,
                           public nsIProtocolHandler,
                           public nsIInputStreamCallback,
                           public nsIOutputStreamCallback,
                           public nsITimerCallback,
                           public nsIDNSListener,
                           public nsIInterfaceRequestor,
                           public nsIChannelEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBSOCKETPROTOCOL
  NS_DECL_NSIHTTPUPGRADELISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIPROTOCOLHANDLER
  NS_DECL_NSIINPUTSTREAMCALLBACK
  NS_DECL_NSIOUTPUTSTREAMCALLBACK
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIDNSLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  nsWebSocketHandler();
  static void Shutdown();
  
  enum {
      
      kContinuation = 0x0,
      kText =         0x1,
      kBinary =       0x2,

      
      kClose =        0x8,
      kPing =         0x9,
      kPong =         0xA
  };
  
  const static PRUint32 kControlFrameMask = 0x8;
  const static PRInt32 kDefaultWSPort     = 80;
  const static PRInt32 kDefaultWSSPort    = 443;
  const static PRUint8 kMaskBit           = 0x80;
  const static PRUint8 kFinalFragBit      = 0x80;

  
  const static PRUint16 kCloseNormal        = 1000;
  const static PRUint16 kCloseGoingAway     = 1001;
  const static PRUint16 kCloseProtocolError = 1002;
  const static PRUint16 kCloseUnsupported   = 1003;
  const static PRUint16 kCloseTooLarge      = 1004;

protected:
  virtual ~nsWebSocketHandler();
  PRBool  mEncrypted;
  
private:
  friend class nsPostMessage;
  friend class nsWSAdmissionManager;
  friend class WSCallOnInputStreamReady;
  
  void SendMsgInternal(nsCString *aMsg, PRInt32 datalen);
  void PrimeNewOutgoingMessage();
  void GeneratePong(PRUint8 *payload, PRUint32 len);
  void GeneratePing();

  nsresult BeginOpen();
  nsresult HandleExtensions();
  nsresult SetupRequest();
  nsresult ApplyForAdmission();
  
  void StopSession(nsresult reason);
  void AbortSession(nsresult reason);
  void ReleaseSession();

  void EnsureHdrOut(PRUint32 size);
  void ApplyMask(PRUint32 mask, PRUint8 *data, PRUint64 len);

  PRBool   IsPersistentFramePtr();
  nsresult ProcessInput(PRUint8 *buffer, PRUint32 count);
  PRUint32 UpdateReadBuffer(PRUint8 *buffer, PRUint32 count);

  class OutboundMessage
  {
  public:
      OutboundMessage (nsCString *str)
          : mMsg(str), mIsControl(PR_FALSE), mBinaryLen(-1) {}

      OutboundMessage (nsCString *str, PRInt32 dataLen)
          : mMsg(str), mIsControl(PR_FALSE), mBinaryLen(dataLen) {}

      OutboundMessage ()
          : mMsg(nsnull), mIsControl(PR_TRUE), mBinaryLen(-1) {}

      ~OutboundMessage() { delete mMsg; }
      
      PRBool IsControl()  { return mIsControl; }
      const nsCString *Msg()  { return mMsg; }
      PRInt32 BinaryLen() { return mBinaryLen; }
      PRInt32 Length() 
      { 
          if (mBinaryLen >= 0)
              return mBinaryLen;
          return mMsg ? mMsg->Length() : 0;
      }
      PRUint8 *BeginWriting() 
      { return (PRUint8 *)(mMsg ? mMsg->BeginWriting() : nsnull); }
      PRUint8 *BeginReading() 
      { return (PRUint8 *)(mMsg ? mMsg->BeginReading() : nsnull); }

  private:
      nsCString *mMsg;
      PRBool     mIsControl;
      PRInt32    mBinaryLen;
  };
  
  nsCOMPtr<nsIURI>                         mOriginalURI;
  nsCOMPtr<nsIURI>                         mURI;
  nsCOMPtr<nsIWebSocketListener>           mListener;
  nsCOMPtr<nsISupports>                    mContext;
  nsCOMPtr<nsIInterfaceRequestor>          mCallbacks;
  nsCOMPtr<nsIEventTarget>                 mSocketThread;
  nsCOMPtr<nsIHttpChannelInternal>         mChannel;
  nsCOMPtr<nsIHttpChannel>                 mHttpChannel;
  nsCOMPtr<nsILoadGroup>                   mLoadGroup;
  nsCOMPtr<nsICancelable>                  mDNSRequest;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  
  nsCString                       mProtocol;
  nsCString                       mOrigin;
  nsCString                       mHashedSecret;
  nsCString                       mAddress;

  nsCOMPtr<nsISocketTransport>    mTransport;
  nsCOMPtr<nsIAsyncInputStream>   mSocketIn;
  nsCOMPtr<nsIAsyncOutputStream>  mSocketOut;

  nsCOMPtr<nsITimer>              mCloseTimer;
  PRUint32                        mCloseTimeout;  

  nsCOMPtr<nsITimer>              mOpenTimer;
  PRUint32                        mOpenTimeout;  

  nsCOMPtr<nsITimer>              mPingTimer;
  PRUint32                        mPingTimeout;  
  PRUint32                        mPingResponseTimeout;  
  
  PRUint32                        mRecvdHttpOnStartRequest   : 1;
  PRUint32                        mRecvdHttpUpgradeTransport : 1;
  PRUint32                        mRequestedClose            : 1;
  PRUint32                        mClientClosed              : 1;
  PRUint32                        mServerClosed              : 1;
  PRUint32                        mStopped                   : 1;
  PRUint32                        mCalledOnStop              : 1;
  PRUint32                        mPingOutstanding           : 1;
  PRUint32                        mAllowCompression          : 1;
  PRUint32                        mAutoFollowRedirects       : 1;
  PRUint32                        mReleaseOnTransmit         : 1;
  
  PRInt32                         mMaxMessageSize;
  nsresult                        mStopOnClose;

  
  PRUint8                        *mFramePtr;
  PRUint8                        *mBuffer;
  PRUint8                         mFragmentOpcode;
  PRUint32                        mFragmentAccumulator;
  PRUint32                        mBuffered;
  PRUint32                        mBufferSize;
  nsCOMPtr<nsIStreamListener>     mInflateReader;
  nsCOMPtr<nsIStringInputStream>  mInflateStream;

  
  const static PRInt32 kCopyBreak = 1000;
  
  OutboundMessage                *mCurrentOut;
  PRUint32                        mCurrentOutSent;
  nsDeque                         mOutgoingMessages;
  nsDeque                         mOutgoingPingMessages;
  nsDeque                         mOutgoingPongMessages;
  PRUint32                        mHdrOutToSend;
  PRUint8                        *mHdrOut;
  PRUint8                         mOutHeader[kCopyBreak + 16];
  nsWSCompression                *mCompressor;
  PRUint32                        mDynamicOutputSize;
  PRUint8                        *mDynamicOutput;
};

class nsWebSocketSSLHandler : public nsWebSocketHandler
{
public:
    nsWebSocketSSLHandler() {nsWebSocketHandler::mEncrypted = PR_TRUE;}
protected:
    virtual ~nsWebSocketSSLHandler() {}
};

}} 
