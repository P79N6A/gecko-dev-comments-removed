






































#include "nsWebSocket.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMWindow.h"
#include "nsIDocument.h"
#include "nsXPCOM.h"
#include "nsIXPConnect.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsDOMError.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIDOMClassInfo.h"
#include "nsDOMClassInfo.h"
#include "jsapi.h"
#include "nsNetUtil.h"
#include "nsIStandardURL.h"
#include "nsIURL.h"
#include "nsIPrivateDOMEvent.h"
#include "nsISocketTransportService.h"
#include "nsIProtocolProxyCallback.h"
#include "nsISocketTransport.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsICancelable.h"
#include "nsIInterfaceRequestor.h"
#include "nsISSLSocketControl.h"
#include "nsISocketProviderService.h"
#include "nsIProtocolProxyService2.h"
#include "nsISocketProvider.h"
#include "nsDeque.h"
#include "nsICookieService.h"
#include "nsICharsetConverterManager.h"
#include "nsIUnicodeEncoder.h"
#include "nsThreadUtils.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMMessageEvent.h"
#include "nsIStandardURL.h"
#include "nsIPromptFactory.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsIStringBundle.h"
#include "nsIConsoleService.h"
#include "nsITimer.h"
#include "nsIDNSListener.h"
#include "nsIDNSRecord.h"
#include "nsIDNSService.h"
#include "nsLayoutStatics.h"
#include "nsIHttpAuthenticableChannel.h"
#include "nsIHttpChannelAuthProvider.h"
#include "mozilla/Mutex.h"

using namespace mozilla;

static nsIThread *gWebSocketThread = nsnull;





#define DEFAULT_BUFFER_SIZE 2048
#define UTF_8_REPLACEMENT_CHAR    static_cast<PRUnichar>(0xFFFD)

#define TIMEOUT_TRY_CONNECT_AGAIN 1000
#define TIMEOUT_WAIT_FOR_SERVER_RESPONSE 20000

#define ENSURE_TRUE_AND_FAIL_IF_FAILED(x, ret)                            \
  PR_BEGIN_MACRO                                                          \
    if (NS_UNLIKELY(!(x))) {                                              \
       NS_WARNING("ENSURE_TRUE_AND_FAIL_IF_FAILED(" #x ") failed");       \
       FailConnection();                                                  \
       return ret;                                                        \
    }                                                                     \
  PR_END_MACRO

#define ENSURE_SUCCESS_AND_FAIL_IF_FAILED(res, ret)                       \
  PR_BEGIN_MACRO                                                          \
    nsresult __rv = res;                                                  \
    if (NS_FAILED(__rv)) {                                                \
      NS_ENSURE_SUCCESS_BODY(res, ret)                                    \
      FailConnection();                                                   \
      return ret;                                                         \
    }                                                                     \
  PR_END_MACRO

#define CHECK_TRUE_AND_FAIL_IF_FAILED(x)                                  \
  PR_BEGIN_MACRO                                                          \
    if (NS_UNLIKELY(!(x))) {                                              \
       NS_WARNING("CHECK_TRUE_AND_FAIL_IF_FAILED(" #x ") failed");        \
       FailConnection();                                                  \
       return;                                                            \
    }                                                                     \
  PR_END_MACRO

#define CHECK_SUCCESS_AND_FAIL_IF_FAILED(res)                             \
  PR_BEGIN_MACRO                                                          \
    nsresult __rv = res;                                                  \
    if (NS_FAILED(__rv)) {                                                \
      NS_ENSURE_SUCCESS_BODY(res, ret)                                    \
      FailConnection();                                                   \
      return;                                                             \
    }                                                                     \
  PR_END_MACRO

#define CHECK_SUCCESS_AND_FAIL_IF_FAILED2(res)                            \
  PR_BEGIN_MACRO                                                          \
    nsresult __rv = res;                                                  \
    if (NS_FAILED(__rv)) {                                                \
      NS_ENSURE_SUCCESS_BODY(res, ret)                                    \
      thisObject->FailConnection();                                       \
      return;                                                             \
    }                                                                     \
  PR_END_MACRO

#define WARN_IF_FALSE_AND_RETURN(_expr, _msg)                             \
  if (!(_expr)) {                                                         \
    NS_WARNING(_msg);                                                     \
    return;                                                               \
  }

#define DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(_method)                      \
  nsresult _method();                                                     \
  void MainRunnable##_method();

#define IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(_method)                \
  nsresult                                                                \
  nsWebSocketEstablishedConnection::_method()                             \
  {                                                                       \
    if (!NS_IsMainThread()) {                                             \
      nsCOMPtr<nsIRunnable> event =                                       \
        NS_NewRunnableMethod(this, &nsWebSocketEstablishedConnection::    \
                                    MainRunnable##_method);               \
      if (!event) {                                                       \
        return NS_ERROR_OUT_OF_MEMORY;                                    \
      }                                                                   \
      return NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);          \
    }                                                                     \
    MainRunnable##_method();                                              \
    return NS_OK;                                                         \
  }                                                                       \
                                                                          \
  void                                                                    \
  nsWebSocketEstablishedConnection::MainRunnable##_method()               \
  {                                                                       \
    if (!mOwner) {                                                        \
      return;                                                             \
    }

#define IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END                           \
  }


#define IS_HIGH_BIT_OF_FRAME_TYPE_SET(_v) ((_v & 0x80) == 0x80)
#define IS_HIGH_BIT_OF_BYTE_SET(_v) ((_v & 0x80) == 0x80)
#define START_BYTE_OF_MESSAGE 0x00
#define END_BYTE_OF_MESSAGE 0xff


class nsWebSocketEstablishedConnection: public nsIInterfaceRequestor,
                                        public nsIDNSListener,
                                        public nsIProtocolProxyCallback,
                                        public nsIInputStreamCallback,
                                        public nsIOutputStreamCallback,
                                        public nsIChannel,
                                        public nsIHttpAuthenticableChannel
{
friend class nsNetAddressComparator;
friend class nsAutoCloseOwner;

public:
  nsWebSocketEstablishedConnection();
  virtual ~nsWebSocketEstablishedConnection();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDNSLISTENER
  NS_DECL_NSIPROTOCOLPROXYCALLBACK
  NS_DECL_NSIINPUTSTREAMCALLBACK
  NS_DECL_NSIOUTPUTSTREAMCALLBACK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIPROXIEDCHANNEL

  
  
  
  NS_IMETHOD GetRequestMethod(nsACString &method);
  NS_IMETHOD GetIsSSL(PRBool *aIsSSL);
  NS_IMETHOD GetProxyMethodIsConnect(PRBool *aProxyMethodIsConnect);
  NS_IMETHOD GetServerResponseHeader(nsACString & aServerResponseHeader);
  NS_IMETHOD GetProxyChallenges(nsACString & aChallenges);
  NS_IMETHOD GetWWWChallenges(nsACString & aChallenges);
  NS_IMETHOD SetProxyCredentials(const nsACString & aCredentials);
  NS_IMETHOD SetWWWCredentials(const nsACString & aCredentials);
  NS_IMETHOD OnAuthAvailable();
  NS_IMETHOD OnAuthCancelled(PRBool userCancel);

  nsresult Init(nsWebSocket *aOwner);
  nsresult Disconnect();

  nsresult PostMessage(const nsString& aMessage);
  PRUint32 GetOutgoingBufferedAmount() { return mOutgoingBufferedAmount; }

  
  static nsTArray<nsRefPtr<nsWebSocketEstablishedConnection> >* sWSsConnecting;

private:
  
  
  
  
  static void TryConnect(nsITimer *aTimer, void *aClosure);

  
  
  static void TimerInitialServerResponseCallback(nsITimer *aTimer,
                                                 void     *aClosure);

  nsresult DoConnect();
  nsresult HandleNewInputString(PRUint32 aStart);
  nsresult AddAuthorizationHeaders(nsCString& aAuthHeaderStr,
                                   PRBool     aIsProxyAuth);
  nsresult AddCookiesToRequest(nsCString& aAuthHeaderStr);
  PRBool UsingHttpProxy();
  nsresult Reset();
  void RemoveFromLoadGroup();
  nsresult ProcessHeaders();
  nsresult PostData(nsCString *aBuffer, PRBool aIsMessage);
  nsresult PrintErrorOnConsole(const char       *aBundleURI,
                               const PRUnichar  *aError,
                               const PRUnichar **aFormatStrings,
                               PRUint32          aFormatStringsLen);

  
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(ProcessAuthentication)

  
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(AddWSConnecting)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(RemoveWSConnecting)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(HandleSetCookieHeader)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(DoInitialRequest)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(FailConnection)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(Connected)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(CloseOwner)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(DispatchNewMessage)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(Retry)
  DECL_RUNNABLE_ON_MAIN_THREAD_METHOD(ResolveNextProxyAndConnect)

  
  nsresult ProxyStartSSL();

  
  Mutex   mLockDisconnect;
  Mutex   mLockOutgoingMessages;
  Mutex   mLockReceivedMessages;
  nsCOMPtr<nsISocketTransport>    mSocketTransport;
  nsCOMPtr<nsIAsyncInputStream>   mSocketInput;
  nsCOMPtr<nsIAsyncOutputStream>  mSocketOutput;
  nsCOMPtr<nsIProxyInfo>          mProxyInfo;
  nsDeque mOutgoingMessages; 
  PRUint32 mBytesAlreadySentOfFirstOutString;
  PRUint32 mOutgoingBufferedAmount; 
                                    
  nsDeque mReceivedMessages; 
                             
  nsWebSocket* mOwner; 

  
  enum {
    kWebSocketOriginPos = 0,
    kWebSocketLocationPos,
    kWebSocketProtocolPos,
    kSetCookiePos,
    kSetCookie2Pos,
    kProxyAuthenticatePos,
    kServerPos,  
    kHeadersLen
  };

  
  nsCString mBuffer;
  PRUint32 mBytesInBuffer; 
                           
  nsCString mHeaders[kHeadersLen];
  PRUint32 mLengthToDiscard;
  PRPackedBool mReadingProxyConnectResponse;

  
  
  enum ProxyConfig {
    eNotResolvingProxy,
    eResolvingSOCKSProxy,
    eResolvingHTTPSProxy,
    eResolvingHTTPProxy,
    eResolvingProxyFailed
  };
  ProxyConfig mCurrentProxyConfig;

  nsresult mProxyFailureReason;
  nsCOMPtr<nsICancelable> mProxyResolveCancelable;
  nsCOMPtr<nsICancelable> mDNSRequest;
  PRNetAddr mPRNetAddr;

  nsCOMPtr<nsITimer> mTryConnectTimer;
  nsCOMPtr<nsITimer> mInitialServerResponseTimer;

  
  nsCString mProxyCredentials;
  nsCString mCredentials;
  PRPackedBool mAuthenticating;
  nsCOMPtr<nsIHttpChannelAuthProvider> mAuthProvider;

  
  nsCString mRequestName;
  nsresult mFailureStatus;

  




































































  enum ConnectionStatus {
    CONN_NOT_CONNECTED,
    CONN_RETRYING_TO_AUTHENTICATE,
    CONN_CONNECTING_TO_HTTP_PROXY,
    CONN_SENDING_INITIAL_REQUEST,
    CONN_WAITING_RESPONSE_FOR_INITIAL_REQUEST,
    CONN_READING_FIRST_CHAR_OF_RESPONSE_HEADER_NAME,
    CONN_READING_RESPONSE_HEADER_NAME,
    CONN_READING_RESPONSE_HEADER_VALUE,
    CONN_WAITING_LF_CHAR_OF_RESPONSE_HEADER,
    CONN_WAITING_LF_CHAR_TO_CONNECTING,
    CONN_CONNECTED_AND_READY,
    CONN_HIGH_BIT_OF_FRAME_TYPE_SET,
    CONN_READING_AND_DISCARDING_LENGTH_BYTES,
    CONN_HIGH_BIT_OF_FRAME_TYPE_NOT_SET,
    CONN_CLOSED
  };
  ConnectionStatus mStatus;
};

nsTArray<nsRefPtr<nsWebSocketEstablishedConnection> >*
  nsWebSocketEstablishedConnection::sWSsConnecting = nsnull;





class nsNetAddressComparator
{
public:
  
  
  PRBool Equals(nsWebSocketEstablishedConnection* a,
                nsWebSocketEstablishedConnection* b) const;
  PRBool LessThan(nsWebSocketEstablishedConnection* a,
                  nsWebSocketEstablishedConnection* b) const;
};

class nsAutoCloseOwner
{
public:
  nsAutoCloseOwner(nsWebSocketEstablishedConnection* conn) : mConnection(conn)
  {}

  ~nsAutoCloseOwner() { mConnection->CloseOwner(); }

private:
  nsWebSocketEstablishedConnection* mConnection;
};

PRBool
nsNetAddressComparator::Equals(nsWebSocketEstablishedConnection* a,
                               nsWebSocketEstablishedConnection* b) const
{
  NS_ASSERTION(a->mOwner && b->mOwner, "Unexpected disconnected connection");

  if ((a->mProxyInfo && !b->mProxyInfo) ||
      (!a->mProxyInfo && b->mProxyInfo)) {
    return PR_FALSE;
  }

  if (a->mProxyInfo) {
    return a->mOwner->mAsciiHost.Equals(b->mOwner->mAsciiHost);
  }

  if (a->mPRNetAddr.raw.family != b->mPRNetAddr.raw.family) {
    return PR_FALSE;
  }

  if (a->mPRNetAddr.raw.family == PR_AF_INET) {
    return a->mPRNetAddr.inet.ip == b->mPRNetAddr.inet.ip;
  }

  NS_ASSERTION(a->mPRNetAddr.raw.family == PR_AF_INET6,
               "Invalid net raw family");

  return a->mPRNetAddr.ipv6.ip.pr_s6_addr64[0] ==
           b->mPRNetAddr.ipv6.ip.pr_s6_addr64[0] &&
         a->mPRNetAddr.ipv6.ip.pr_s6_addr64[1] ==
           b->mPRNetAddr.ipv6.ip.pr_s6_addr64[1];
}

PRBool
nsNetAddressComparator::LessThan(nsWebSocketEstablishedConnection* a,
                                 nsWebSocketEstablishedConnection* b) const
{
  NS_ASSERTION(a->mOwner && b->mOwner, "Unexpected disconnected connection");

  if (a->mProxyInfo && !b->mProxyInfo) {
    return PR_FALSE;
  }

  if (!a->mProxyInfo && b->mProxyInfo) {
    return PR_TRUE;
  }

  if (a->mProxyInfo) {
    return (a->mOwner->mAsciiHost < b->mOwner->mAsciiHost);
  }

  if (a->mPRNetAddr.raw.family == PR_AF_INET &&
      b->mPRNetAddr.raw.family == PR_AF_INET6) {
    return PR_TRUE;
  }

  if (a->mPRNetAddr.raw.family == PR_AF_INET6 &&
      b->mPRNetAddr.raw.family == PR_AF_INET) {
    return PR_FALSE;
  }

  if (a->mPRNetAddr.raw.family == PR_AF_INET &&
      b->mPRNetAddr.raw.family == PR_AF_INET) {
    return a->mPRNetAddr.inet.ip < b->mPRNetAddr.inet.ip;
  }

  NS_ASSERTION(a->mPRNetAddr.raw.family == PR_AF_INET6 &&
               b->mPRNetAddr.raw.family == PR_AF_INET6,
               "Invalid net raw family");

  return a->mPRNetAddr.ipv6.ip.pr_s6_addr64[0] <
           b->mPRNetAddr.ipv6.ip.pr_s6_addr64[0] ||
         (a->mPRNetAddr.ipv6.ip.pr_s6_addr64[0] ==
           b->mPRNetAddr.ipv6.ip.pr_s6_addr64[0] &&
          a->mPRNetAddr.ipv6.ip.pr_s6_addr64[1] <
           b->mPRNetAddr.ipv6.ip.pr_s6_addr64[1]);
}





NS_IMPL_THREADSAFE_ISUPPORTS9(nsWebSocketEstablishedConnection,
                              nsIInterfaceRequestor,
                              nsIDNSListener,
                              nsIProtocolProxyCallback,
                              nsIInputStreamCallback,
                              nsIOutputStreamCallback,
                              nsIRequest,
                              nsIChannel,
                              nsIProxiedChannel,
                              nsIHttpAuthenticableChannel)





nsWebSocketEstablishedConnection::nsWebSocketEstablishedConnection() :
  mLockDisconnect("WebSocket's disconnect lock"),
  mLockOutgoingMessages("WebSocket's outgoing messages lock"),
  mLockReceivedMessages("WebSocket's received messages lock"),
  mBytesAlreadySentOfFirstOutString(0),
  mOutgoingBufferedAmount(0),
  mOwner(nsnull),
  mBytesInBuffer(0),
  mLengthToDiscard(0),
  mReadingProxyConnectResponse(PR_FALSE),
  mCurrentProxyConfig(eNotResolvingProxy),
  mProxyFailureReason(NS_OK),
  mAuthenticating(PR_FALSE),
  mFailureStatus(NS_OK),
  mStatus(CONN_NOT_CONNECTED)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");
  nsLayoutStatics::AddRef();
}

nsWebSocketEstablishedConnection::~nsWebSocketEstablishedConnection()
{
}

nsresult
nsWebSocketEstablishedConnection::PostData(nsCString *aBuffer,
                                           PRBool aIsMessage)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  MutexAutoLock lockOut(mLockOutgoingMessages);

  nsresult rv;
  PRInt32 sizeBefore = mOutgoingMessages.GetSize();
  mOutgoingMessages.Push(aBuffer);
  NS_ENSURE_TRUE(mOutgoingMessages.GetSize() == sizeBefore + 1,
                 NS_ERROR_OUT_OF_MEMORY);
  if (aIsMessage) {
    
    mOutgoingBufferedAmount += aBuffer->Length() - 2;
  }

  if (sizeBefore == 0) {
    mBytesAlreadySentOfFirstOutString = 0;
    rv = mSocketOutput->AsyncWait(this, 0, 0, gWebSocketThread);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsWebSocketEstablishedConnection::PostMessage(const nsString& aMessage)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  if (!mOwner) {
    return NS_OK;
  }

  
  NS_ENSURE_STATE(mStatus >= CONN_CONNECTED_AND_READY);

  nsresult rv;

  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicodeEncoder> converter;
  rv = ccm->GetUnicodeEncoder("UTF-8", getter_AddRefs(converter));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = converter->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace,
                                         nsnull, UTF_8_REPLACEMENT_CHAR);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 inLen = aMessage.Length();
  PRInt32 maxLen;
  rv = converter->GetMaxLength(aMessage.BeginReading(), inLen, &maxLen);
  NS_ENSURE_SUCCESS(rv, rv);
  maxLen += 2;   

  nsAutoPtr<nsCString> buf(new nsCString());
  NS_ENSURE_TRUE(buf.get(), NS_ERROR_OUT_OF_MEMORY);

  buf->SetLength(maxLen);
  NS_ENSURE_TRUE(buf->Length() == static_cast<PRUint32>(maxLen),
                 NS_ERROR_OUT_OF_MEMORY);

  char* start = buf->BeginWriting();
  *start = static_cast<char>(START_BYTE_OF_MESSAGE);
  ++start;

  PRInt32 outLen = maxLen;
  rv = converter->Convert(aMessage.BeginReading(), &inLen, start, &outLen);
  if (NS_SUCCEEDED(rv)) {
    PRInt32 outLen2 = maxLen - outLen;
    rv = converter->Finish(start + outLen, &outLen2);
    outLen += outLen2;
  }
  if (NS_FAILED(rv) || rv == NS_ERROR_UENC_NOMAPPING) {
    
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  char* end = buf->BeginWriting() + outLen + 1;
  *end = static_cast<char>(END_BYTE_OF_MESSAGE);

  outLen += 2;

  buf->SetLength(outLen);
  NS_ENSURE_TRUE(buf->Length() == static_cast<PRUint32>(outLen),
                 NS_ERROR_UNEXPECTED);

  return PostData(buf.forget(), PR_TRUE);
}

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(DoInitialRequest)
{
  nsresult rv;
  nsString strRequestTmp;

  nsAutoPtr<nsCString> buf(new nsCString());
  CHECK_TRUE_AND_FAIL_IF_FAILED(buf.get());

  
  buf->AppendLiteral("GET ");
  buf->Append(mOwner->mResource);
  buf->AppendLiteral(" HTTP/1.1\r\n"
                     "Upgrade: WebSocket\r\n"
                     "Connection: Upgrade\r\n");

  
  buf->AppendLiteral("Host: ");
  buf->Append(mOwner->mAsciiHost);
  buf->AppendLiteral(":");
  buf->AppendInt(mOwner->mPort);
  buf->AppendLiteral("\r\n");

  
  buf->AppendLiteral("Origin: ");
  buf->Append(mOwner->mOrigin);
  buf->AppendLiteral("\r\n");
  
  if (!mOwner->mProtocol.IsEmpty()) {
    buf->AppendLiteral("WebSocket-Protocol: ");
    buf->Append(mOwner->mProtocol);
    buf->AppendLiteral("\r\n");
  }
  
  rv = AddAuthorizationHeaders(*buf, PR_FALSE);
  CHECK_SUCCESS_AND_FAIL_IF_FAILED(rv);
  
  rv = AddCookiesToRequest(*buf);
  CHECK_SUCCESS_AND_FAIL_IF_FAILED(rv);
  
  buf->AppendLiteral("\r\n");

  mStatus = CONN_SENDING_INITIAL_REQUEST;

  rv = PostData(buf.forget(), PR_FALSE);
  CHECK_SUCCESS_AND_FAIL_IF_FAILED(rv);
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

static
nsresult
GetHttpResponseCode(const nsCString& aLine, PRUint32 aLen,
                    PRUint32 *aStatusCode, PRUint32 *aLineLen)
{
  
  if (aLen < 4) {
    return NS_ERROR_IN_PROGRESS;
  }
  if (!StringBeginsWith(aLine, NS_LITERAL_CSTRING("HTTP"))) {
    return NS_ERROR_UNEXPECTED;
  }

  
  PRUint32 responseCode = 0;
  PRUint8 responseCodeReadingState = 0; 
                                        
  char last2Chrs[2] = {'\0', '\0'};
  PRUint32 i = 4;  
  for (; i < aLen; ++i) {
    if (responseCodeReadingState == 0 && aLine[i] == ' ') {
      responseCodeReadingState = 1;
    } else if (responseCodeReadingState == 1) {
      if (aLine[i] == ' ') {
        responseCodeReadingState = 2;
      } else if (aLine[i] >= '0' && aLine[i] <= '9') {
        responseCode = 10 * responseCode + (aLine[i] - '0');
      } else {
        return NS_ERROR_UNEXPECTED;
      }
    }

    last2Chrs[0] = last2Chrs[1];
    last2Chrs[1] = aLine[i];
    if (last2Chrs[0] == '\r' && last2Chrs[1] == '\n') { 
      *aStatusCode = responseCode;
      *aLineLen = i + 1;
      return NS_OK;
    }
  }

  return NS_ERROR_IN_PROGRESS;
}

nsresult
nsWebSocketEstablishedConnection::HandleNewInputString(PRUint32 aStart)
{
  NS_ASSERTION(!NS_IsMainThread(), "Not running on socket thread");

  if (mBytesInBuffer == 0 || aStart == mBytesInBuffer) {
    return NS_OK;
  }

  NS_ENSURE_STATE(aStart < mBytesInBuffer);

  nsresult rv;

  switch (mStatus)
  {
    case CONN_CONNECTING_TO_HTTP_PROXY:
    {
      PRUint32 statusCode, lengthStr;

      rv = GetHttpResponseCode(mBuffer, mBytesInBuffer, &statusCode,
                               &lengthStr);
      if (rv != NS_ERROR_IN_PROGRESS) {
        NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

        if (statusCode == 200) {
          mReadingProxyConnectResponse = PR_TRUE;
          mAuthenticating = PR_FALSE;
        } else if (statusCode == 407) {
          mReadingProxyConnectResponse = PR_TRUE;
          mAuthenticating = PR_TRUE;
        } else {
          return NS_ERROR_UNEXPECTED;
        }

        mStatus = CONN_READING_FIRST_CHAR_OF_RESPONSE_HEADER_NAME;
        mBuffer.Cut(0, lengthStr);
        mBytesInBuffer -= lengthStr;

        return HandleNewInputString(0);
      }
    }
    break;

    case CONN_WAITING_RESPONSE_FOR_INITIAL_REQUEST:
    {
      const char strResponseCheck[] =
        "HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
        "Upgrade: WebSocket\r\n"
        "Connection: Upgrade\r\n";
      PRUint32 lengthStr = NS_ARRAY_LENGTH(strResponseCheck) - 1;

      if (mBytesInBuffer < lengthStr) {
        return NS_OK;
      }

      if (memcmp(mBuffer.BeginReading(), strResponseCheck, lengthStr)) {
        return NS_ERROR_UNEXPECTED;
      }

      mReadingProxyConnectResponse = PR_FALSE;
      mAuthenticating = PR_FALSE;

      mStatus = CONN_READING_FIRST_CHAR_OF_RESPONSE_HEADER_NAME;
      mBuffer.Cut(0, lengthStr);
      mBytesInBuffer -= lengthStr;

      
      
      mInitialServerResponseTimer->Cancel();

      return HandleNewInputString(0);
    }
    break;

    case CONN_READING_FIRST_CHAR_OF_RESPONSE_HEADER_NAME:
    {
      if (mBuffer[aStart] == '\r') {
        mStatus = CONN_WAITING_LF_CHAR_TO_CONNECTING;
        return HandleNewInputString(aStart + 1);
      }

      NS_ENSURE_STATE(mBuffer[aStart] != '\n');

      mStatus = CONN_READING_RESPONSE_HEADER_NAME;
      return HandleNewInputString(aStart);
    }
    break;

    case CONN_READING_RESPONSE_HEADER_NAME:
    {
      PRUint32 i;
      for (i = aStart; i < mBytesInBuffer; ++i) {
        NS_ENSURE_STATE(mBuffer[i] != '\r' && mBuffer[i] != '\n');

        if (mBuffer[i] == ':') {
          mStatus = CONN_READING_RESPONSE_HEADER_VALUE;
          return HandleNewInputString(i + 1);
        }
      }
    }
    break;

    case CONN_READING_RESPONSE_HEADER_VALUE:
    {
      PRUint32 i;
      for (i = aStart; i < mBytesInBuffer; ++i) {
        if (mBuffer[i] == '\r') {
          mStatus = CONN_WAITING_LF_CHAR_OF_RESPONSE_HEADER;
          return HandleNewInputString(i + 1);
        }

        NS_ENSURE_STATE(mBuffer[i] != '\n');
      }
    }
    break;

    case CONN_WAITING_LF_CHAR_OF_RESPONSE_HEADER:
    {
      NS_ENSURE_STATE(mBuffer[aStart] == '\n');

      PRUint32 posColon = mBuffer.FindChar(':');
      PRUint32 posCR = mBuffer.FindChar('\r');

      const nsCSubstring& headerName = Substring(mBuffer, 0, posColon);

      nsCString headerValue;
      if (mBuffer[posColon + 1] == 0x20 && posColon + 2 != posCR) {
        headerValue = Substring(mBuffer, posColon + 2, posCR - posColon - 2);
      } else if (posColon + 1 != posCR) {
        headerValue = Substring(mBuffer, posColon + 1, posCR - posColon - 1);
      } else {
        ; 
      }

      NS_ENSURE_STATE(!headerName.IsEmpty());

      PRInt32 headerPos = -1;
      if (mReadingProxyConnectResponse) {
        if (headerName.LowerCaseEqualsLiteral("proxy-authenticate")) {
          headerPos = kProxyAuthenticatePos;
        }
      } else {
        if (headerName.LowerCaseEqualsLiteral("websocket-origin")) {
          headerPos = kWebSocketOriginPos;
        } else if (headerName.LowerCaseEqualsLiteral("websocket-location")) {
          headerPos = kWebSocketLocationPos;
        } else if (headerName.LowerCaseEqualsLiteral("websocket-protocol")) {
          headerPos = kWebSocketProtocolPos;
        } else if (headerName.LowerCaseEqualsLiteral("set-cookie")) {
          headerPos = kSetCookiePos;
        } else if (headerName.LowerCaseEqualsLiteral("set-cookie2")) {
          headerPos = kSetCookie2Pos;
        }
      }
      if (headerPos == -1 && headerName.LowerCaseEqualsLiteral("server")) {
        headerPos = kServerPos;
      }

      if (headerPos != -1) {
        NS_ENSURE_STATE(mHeaders[headerPos].IsEmpty());
        mHeaders[headerPos] = headerValue;
      }

      mStatus = CONN_READING_FIRST_CHAR_OF_RESPONSE_HEADER_NAME;
      mBuffer.Cut(0, aStart + 1);
      mBytesInBuffer -= aStart + 1;

      return HandleNewInputString(0);
    }
    break;

    case CONN_WAITING_LF_CHAR_TO_CONNECTING:
    {
      NS_ENSURE_STATE(mBuffer[aStart] == '\n');

      rv = ProcessHeaders();
      NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

      if (mAuthenticating) {
        Reset();
        mStatus = CONN_RETRYING_TO_AUTHENTICATE;
        return ProcessAuthentication();
      }

      if (mReadingProxyConnectResponse) {
        if (mOwner->mSecure) {
          rv = ProxyStartSSL();
          NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);
        }

        mBytesInBuffer = 0;
        return DoInitialRequest();
      }

      mStatus = CONN_CONNECTED_AND_READY;
      rv = Connected();
      NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

      mBuffer.Cut(0, aStart + 1);
      mBytesInBuffer -= aStart + 1;
      return HandleNewInputString(0);
    }

    case CONN_CONNECTED_AND_READY:
    {
      NS_ENSURE_STATE(aStart == 0);
      PRUint8 frameType = mBuffer[0];

      if (IS_HIGH_BIT_OF_FRAME_TYPE_SET(frameType)) {
        mStatus = CONN_HIGH_BIT_OF_FRAME_TYPE_SET;
        mLengthToDiscard = 0;
      } else {
        mStatus = CONN_HIGH_BIT_OF_FRAME_TYPE_NOT_SET;
      }

      return HandleNewInputString(1);
    }
    break;

    case CONN_HIGH_BIT_OF_FRAME_TYPE_SET:
    {
      PRUint32 i;
      for (i = aStart; i < mBytesInBuffer; ++i) {
        PRUint8 b, bv;
        b = mBuffer[i];
        bv = (b & 0x7f);

        
        NS_ENSURE_STATE(mLengthToDiscard <= ((PR_UINT32_MAX - bv) / 128));

        mLengthToDiscard = mLengthToDiscard * 128 + bv;

        if (!IS_HIGH_BIT_OF_BYTE_SET(b)) {
          mStatus = CONN_READING_AND_DISCARDING_LENGTH_BYTES;
          return HandleNewInputString(i + 1);
        }
      }
      mBytesInBuffer = 0;
    }
    break;

    case CONN_READING_AND_DISCARDING_LENGTH_BYTES:
    {
      if (mBytesInBuffer - aStart >= mLengthToDiscard) {
        mBuffer.Cut(0, aStart + mLengthToDiscard);
        mBytesInBuffer -= aStart + mLengthToDiscard;

        mStatus = CONN_CONNECTED_AND_READY;
        return HandleNewInputString(0);
      }

      mLengthToDiscard -= mBytesInBuffer - aStart;
      mBytesInBuffer = 0;
    }
    break;

    case CONN_HIGH_BIT_OF_FRAME_TYPE_NOT_SET:
    {
      PRUint32 i;
      for (i = aStart; i < mBytesInBuffer; ++i) {
        PRUint8 b;
        b = mBuffer[i];
        if (b == END_BYTE_OF_MESSAGE) {
          PRUint8 frameType = mBuffer[0];
          if (frameType == START_BYTE_OF_MESSAGE) {
            
            
            nsCString* dataMessage = new nsCString();
            NS_ENSURE_TRUE(dataMessage, NS_ERROR_OUT_OF_MEMORY);
            *dataMessage = Substring(mBuffer, 1, i - 1);

            
            {
              MutexAutoLock lockIn(mLockReceivedMessages);

              PRInt32 sizeBefore = mReceivedMessages.GetSize();
              mReceivedMessages.Push(dataMessage);
              NS_ENSURE_TRUE(mReceivedMessages.GetSize() == sizeBefore + 1,
                             NS_ERROR_OUT_OF_MEMORY);
            }

            
            rv = DispatchNewMessage();
            NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);
          }

          mBuffer.Cut(0, i + 1);
          mBytesInBuffer -= i + 1;

          mStatus = CONN_CONNECTED_AND_READY;
          return HandleNewInputString(0);
        }
      }
    }
    break;

    default:
      NS_ASSERTION(PR_FALSE, "Invalid state.");
  }

  return NS_OK;
}

nsresult
nsWebSocketEstablishedConnection::AddAuthorizationHeaders(nsCString& aStr,
                                                          PRBool aIsProxyAuth)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  mAuthProvider->AddAuthorizationHeaders();

  if (UsingHttpProxy() && !mProxyCredentials.IsEmpty()) {
    aStr.AppendLiteral("Proxy-Authorization: ");
    aStr.Append(mProxyCredentials);
    aStr.AppendLiteral("\r\n");
  }

  if (!aIsProxyAuth && !mCredentials.IsEmpty()) {
    aStr.AppendLiteral("Authorization: ");
    aStr.Append(mCredentials);
    aStr.AppendLiteral("\r\n");
  }
  return NS_OK;
}

nsresult
nsWebSocketEstablishedConnection::AddCookiesToRequest(nsCString& aStr)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  
  nsCOMPtr<nsICookieService> cookieService =
    do_GetService(NS_COOKIESERVICE_CONTRACTID);
  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(mOwner->mScriptContext);

  if (!cookieService || !doc) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> documentURI = doc->GetDocumentURI();
  if (!documentURI) {
    return NS_OK;
  }

  nsXPIDLCString cookieValue;
  cookieService->GetCookieStringFromHttp(documentURI,
                                         documentURI,
                                         nsnull,
                                         getter_Copies(cookieValue));
  if (!cookieValue.IsEmpty()) {
    aStr.AppendLiteral("Cookie: ");
    aStr.Append(cookieValue);
    aStr.AppendLiteral("\r\n");
  }

  return NS_OK;
}

PRBool
nsWebSocketEstablishedConnection::UsingHttpProxy()
{
  if (!mProxyInfo) {
    return PR_FALSE;
  }

  nsCAutoString proxyType;
  mProxyInfo->GetType(proxyType);
  return proxyType.EqualsLiteral("http");
}


nsresult
nsWebSocketEstablishedConnection::Reset()
{
  RemoveWSConnecting();

  mStatus = CONN_NOT_CONNECTED;

  if (mSocketTransport) {
    mSocketTransport->Close(NS_OK);
    mSocketTransport = nsnull;
  }
  mSocketInput = nsnull;
  mSocketOutput = nsnull;

  while (mOutgoingMessages.GetSize() != 0) {
    delete static_cast<nsCString*>(mOutgoingMessages.PopFront());
  }

  while (mReceivedMessages.GetSize() != 0) {
    delete static_cast<nsString*>(mReceivedMessages.PopFront());
  }

  mBytesAlreadySentOfFirstOutString = 0;
  mBytesInBuffer = 0;

  return NS_OK;
}

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(Connected)
{
  RemoveWSConnecting();

  if (mAuthProvider) {
    mAuthProvider->Disconnect(NS_ERROR_ABORT);
    mAuthProvider = nsnull;
  }

  mOwner->SetReadyState(nsIWebSocket::OPEN);
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(CloseOwner)
{
  mOwner->Close();
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(DispatchNewMessage)
{
  nsresult rv;

  while (PR_TRUE) {
    nsAutoPtr<nsCString> data;

    {
      MutexAutoLock lockIn(mLockReceivedMessages);

      if (mReceivedMessages.GetSize() == 0) {
        return;
      }

      data = static_cast<nsCString*>(mReceivedMessages.PopFront());
    }

    nsCOMPtr<nsIDocument> doc =
       nsContentUtils::GetDocumentFromScriptContext(mOwner->mScriptContext);
    rv = mOwner->CheckInnerWindowCorrectness();
    if (NS_FAILED(rv) || !doc) {
      continue;
    }

    
    

    nsCOMPtr<nsIDOMEvent> event;
    rv = nsEventDispatcher::CreateEvent(nsnull, nsnull,
                                        NS_LITERAL_STRING("messageevent"),
                                        getter_AddRefs(event));
    if (NS_FAILED(rv)) {
      NS_WARNING("failed creating event");
      return;
    }

    nsCOMPtr<nsIDOMMessageEvent> messageEvent = do_QueryInterface(event);
    rv = messageEvent->InitMessageEvent(NS_LITERAL_STRING("message"),
                                        PR_FALSE, PR_FALSE,
                                        NS_ConvertUTF8toUTF16(*data),
                                        NS_ConvertUTF8toUTF16(mOwner->mOrigin),
                                        EmptyString(), nsnull);
    if (NS_FAILED(rv)) {
      NS_WARNING("failed initializing message event");
      return;
    }

    nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
    rv = privateEvent->SetTrusted(PR_TRUE);
    if (NS_FAILED(rv)) {
      NS_WARNING("failed trusting event");
      return;
    }

    rv = nsEventDispatcher::DispatchDOMEvent(static_cast<nsPIDOMEventTarget*>(mOwner),
                                             nsnull, event, nsnull, nsnull);
    if (NS_FAILED(rv)) {
      NS_WARNING("failed dispatching message event");
      return;
    }
  }
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

nsresult
nsWebSocketEstablishedConnection::ProxyStartSSL()
{
  NS_ASSERTION(!NS_IsMainThread(), "Not running on socket thread");

  nsCOMPtr<nsISupports> securityInfo;
  nsresult rv = mSocketTransport->GetSecurityInfo(getter_AddRefs(securityInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISSLSocketControl> ssl = do_QueryInterface(securityInfo, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return ssl->ProxyStartSSL();
}

nsresult
nsWebSocketEstablishedConnection::Init(nsWebSocket *aOwner)
{
  
  NS_ASSERTION(!mOwner, "WebSocket's connection is already initialized");

  nsresult rv;

  mOwner = aOwner;

  if (mOwner->mSecure) {
    
    nsCOMPtr<nsISocketProviderService> spserv =
      do_GetService(NS_SOCKETPROVIDERSERVICE_CONTRACTID);
    NS_ENSURE_STATE(spserv);

    nsCOMPtr<nsISocketProvider> provider;
    rv = spserv->GetSocketProvider("ssl", getter_AddRefs(provider));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mTryConnectTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialServerResponseTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsILoadGroup> loadGroup;
  rv = GetLoadGroup(getter_AddRefs(loadGroup));
  NS_ENSURE_SUCCESS(rv, rv);
  if (loadGroup) {
    rv = loadGroup->AddRequest(this, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mAuthProvider =
    do_CreateInstance("@mozilla.org/network/http-channel-auth-provider;1",
                      &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mAuthProvider->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  CopyUTF16toUTF8(mOwner->mOriginalURL, mRequestName);

  if (!sWSsConnecting) {
    sWSsConnecting =
      new nsTArray<nsRefPtr<nsWebSocketEstablishedConnection> >();
    ENSURE_TRUE_AND_FAIL_IF_FAILED(sWSsConnecting, NS_ERROR_OUT_OF_MEMORY);
  }

  if (!gWebSocketThread) {
    rv = NS_NewThread(&gWebSocketThread);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = ResolveNextProxyAndConnect();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsWebSocketEstablishedConnection::DoConnect()
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  nsresult rv;

  rv = AddWSConnecting();
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  nsCOMPtr<nsISocketTransportService> sts =
    do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  
  const char* types[1];
  nsAdoptingCString value =
    nsContentUtils::GetCharPref("network.http.default-socket-type");

  if (mOwner->mSecure) {
    types[0] = "ssl";
  } else {
    if (value.IsEmpty()) {
      types[0] = nsnull;
    } else {
      types[0] = value.get();
    }
  }

  nsCOMPtr<nsISocketTransport> strans;
  PRUint32 typeCount = (types[0] != nsnull ? 1 : 0);

  rv = sts->CreateTransport(types, typeCount, mOwner->mAsciiHost, mOwner->mPort,
                            mProxyInfo, getter_AddRefs(strans));
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  rv = strans->SetSecurityCallbacks(this);
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  nsCOMPtr<nsIOutputStream> outStream;
  rv = strans->OpenOutputStream(nsITransport::OPEN_UNBUFFERED, 0, 0,
                                getter_AddRefs(outStream));
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  nsCOMPtr<nsIInputStream> inStream;
  rv = strans->OpenInputStream(nsITransport::OPEN_UNBUFFERED, 0, 0,
                               getter_AddRefs(inStream));
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  mSocketTransport = strans;
  mSocketInput = do_QueryInterface(inStream);
  mSocketOutput = do_QueryInterface(outStream);
  mProxyResolveCancelable = nsnull;

  if (!UsingHttpProxy()) {
    rv = DoInitialRequest();
    ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);
    return NS_OK;
  }

  nsAutoPtr<nsCString> buf(new nsCString());
  NS_ENSURE_TRUE(buf.get(), NS_ERROR_OUT_OF_MEMORY);

  nsString strRequestTmp;

  
  buf->AppendLiteral("CONNECT ");
  buf->Append(mOwner->mAsciiHost);
  buf->AppendLiteral(":");
  buf->AppendInt(mOwner->mPort);
  buf->AppendLiteral(" HTTP/1.1\r\n");

  
  
  
  buf->AppendLiteral("Host: ");
  buf->Append(mOwner->mAsciiHost);
  buf->AppendLiteral(":");
  buf->AppendInt(mOwner->mPort);
  buf->AppendLiteral("\r\n");

  
  rv = AddAuthorizationHeaders(*buf, PR_TRUE);
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  buf->AppendLiteral("\r\n");

  mStatus = CONN_CONNECTING_TO_HTTP_PROXY;

  rv = PostData(buf.forget(), PR_FALSE);
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  return NS_OK;
}

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(AddWSConnecting)
{
#ifdef DEBUG
  PRUint32 index =
    sWSsConnecting->BinaryIndexOf(this, nsNetAddressComparator());
  NS_ASSERTION(index == nsTArray<PRNetAddr>::NoIndex,
               "The ws connection shouldn't be already added in the "
               "serialization list.");
#endif

  PRBool inserted =
    !!(sWSsConnecting->InsertElementSorted(this, nsNetAddressComparator()));
  NS_ASSERTION(inserted, "Couldn't insert the ws connection into the "
                         "serialization list.");
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(RemoveWSConnecting)
{
  PRUint32 index =
    sWSsConnecting->BinaryIndexOf(this, nsNetAddressComparator());
  if (index != nsTArray<PRNetAddr>::NoIndex) {
    sWSsConnecting->RemoveElementAt(index);
  }
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END


void
nsWebSocketEstablishedConnection::TryConnect(nsITimer* aTimer,
                                             void* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  nsresult rv;
  nsRefPtr<nsWebSocketEstablishedConnection> thisObject =
    static_cast<nsWebSocketEstablishedConnection*>(aClosure);

  if (!thisObject->mOwner) { 
    return;
  }

  PRUint32 index = sWSsConnecting->BinaryIndexOf(thisObject,
                                                 nsNetAddressComparator());
  if (index != nsTArray<PRNetAddr>::NoIndex) {
    
    rv = thisObject->mTryConnectTimer->
      InitWithFuncCallback(TryConnect, thisObject,
                           TIMEOUT_TRY_CONNECT_AGAIN, nsITimer::TYPE_ONE_SHOT);
    CHECK_SUCCESS_AND_FAIL_IF_FAILED2(rv);
  } else {
    rv = thisObject->DoConnect();
    CHECK_SUCCESS_AND_FAIL_IF_FAILED2(rv);
  }
}


void
nsWebSocketEstablishedConnection::
  TimerInitialServerResponseCallback(nsITimer* aTimer,
                                     void* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  nsRefPtr<nsWebSocketEstablishedConnection> thisObject =
    static_cast<nsWebSocketEstablishedConnection*>(aClosure);

  if (!thisObject->mOwner) { 
    return;
  }

  thisObject->FailConnection();
}

nsresult
nsWebSocketEstablishedConnection::ProcessHeaders()
{
  NS_ASSERTION(!NS_IsMainThread(), "Not running on socket thread");

  nsresult rv;

  if (mAuthenticating) {
    if (mHeaders[kProxyAuthenticatePos].IsEmpty())
      return NS_ERROR_UNEXPECTED;
    return NS_OK;
  }

  if (mReadingProxyConnectResponse) {
    return NS_OK;
  }

  

  nsCString responseOriginHeader = mHeaders[kWebSocketOriginPos];
  ToLowerCase(responseOriginHeader);

  if (!responseOriginHeader.Equals(mOwner->mOrigin)) {
    return NS_ERROR_UNEXPECTED;
  }

  

  nsCString validWebSocketLocation1, validWebSocketLocation2;
  validWebSocketLocation1.Append(mOwner->mSecure ? "wss://" : "ws://");
  validWebSocketLocation1.Append(mOwner->mAsciiHost);
  validWebSocketLocation1.Append(":");
  validWebSocketLocation1.AppendInt(mOwner->mPort);
  validWebSocketLocation1.Append(mOwner->mResource);

  if ((mOwner->mSecure && mOwner->mPort != DEFAULT_WSS_SCHEME_PORT) ||
      (!mOwner->mSecure && mOwner->mPort != DEFAULT_WS_SCHEME_PORT)) {
    validWebSocketLocation2 = validWebSocketLocation1;
  } else {
    validWebSocketLocation2.Append(mOwner->mSecure ? "wss://" : "ws://");
    validWebSocketLocation2.Append(mOwner->mAsciiHost);
    validWebSocketLocation2.Append(mOwner->mResource);
  }

  if (!mHeaders[kWebSocketLocationPos].Equals(validWebSocketLocation1) &&
      !mHeaders[kWebSocketLocationPos].Equals(validWebSocketLocation2)) {
    return NS_ERROR_UNEXPECTED;
  }

  
  if (!mOwner->mProtocol.IsEmpty() &&
      !mHeaders[kWebSocketProtocolPos].
        Equals(mOwner->mProtocol)) {
    return NS_ERROR_UNEXPECTED;
  }

  

  if (!mHeaders[kSetCookiePos].IsEmpty()) {
    rv = HandleSetCookieHeader();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  

  return NS_OK;
}

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(HandleSetCookieHeader)
{
  nsresult rv;

  nsCOMPtr<nsICookieService> cookieService =
    do_GetService(NS_COOKIESERVICE_CONTRACTID);
  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(mOwner->mScriptContext);

  if (!cookieService || !doc) {
    return;
  }

  nsCOMPtr<nsIURI> documentURI = doc->GetDocumentURI();
  if (!documentURI) {
    return;
  }

  nsCOMPtr<nsIPromptFactory> wwatch =
    do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  CHECK_SUCCESS_AND_FAIL_IF_FAILED(rv);

  nsCOMPtr<nsIPrompt> prompt;
  nsCOMPtr<nsPIDOMWindow> outerWindow = doc->GetWindow();
  rv = wwatch->GetPrompt(outerWindow, NS_GET_IID(nsIPrompt),
                         getter_AddRefs(prompt));
  CHECK_SUCCESS_AND_FAIL_IF_FAILED(rv);

  rv = cookieService->SetCookieStringFromHttp(documentURI,
                                              documentURI,
                                              prompt,
                                              mHeaders[kSetCookiePos].get(),
                                              nsnull,
                                              nsnull);
  CHECK_SUCCESS_AND_FAIL_IF_FAILED(rv);
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

nsresult
nsWebSocketEstablishedConnection::PrintErrorOnConsole(const char *aBundleURI,
                                                      const PRUnichar *aError,
                                                      const PRUnichar **aFormatStrings,
                                                      PRUint32 aFormatStringsLen)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  nsresult rv;

  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> strBundle;
  rv = bundleService->CreateBundle(aBundleURI, getter_AddRefs(strBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIConsoleService> console(
    do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsXPIDLString message;
  if (aFormatStrings) {
    rv = strBundle->FormatStringFromName(aError, aFormatStrings,
                                         aFormatStringsLen,
                                         getter_Copies(message));
  } else {
    rv = strBundle->GetStringFromName(aError, getter_Copies(message));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = console->LogStringMessage(message.get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(FailConnection)
{
  nsresult rv;
  nsAutoCloseOwner autoClose(this);

  if (mFailureStatus == NS_OK) {
    mFailureStatus = NS_ERROR_UNEXPECTED;
  }

  nsCAutoString targetSpec;
  rv = mOwner->mURI->GetSpec(targetSpec);
  WARN_IF_FALSE_AND_RETURN(NS_SUCCEEDED(rv), "Failed to get targetSpec");

  NS_ConvertUTF8toUTF16 specUTF16(targetSpec);
  const PRUnichar *formatStrings[] = { specUTF16.get() };

  if (mStatus < CONN_CONNECTED_AND_READY) {
    if (mCurrentProxyConfig == eResolvingProxyFailed) {
      PrintErrorOnConsole("chrome://browser/locale/appstrings.properties",
                          NS_LITERAL_STRING("proxyConnectFailure").get(),
                          nsnull, 0);
    }
    PrintErrorOnConsole("chrome://browser/locale/appstrings.properties",
                        NS_LITERAL_STRING("connectionFailure").get(),
                        formatStrings, NS_ARRAY_LENGTH(formatStrings));
  } else {
    PrintErrorOnConsole("chrome://browser/locale/appstrings.properties",
                        NS_LITERAL_STRING("netInterrupt").get(),
                        formatStrings, NS_ARRAY_LENGTH(formatStrings));
  }
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

nsresult
nsWebSocketEstablishedConnection::Disconnect()
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  {
    MutexAutoLock lockDisconnect(mLockDisconnect);

    if (!mOwner) {
      return NS_OK;
    }

    RemoveWSConnecting();

    mStatus = CONN_CLOSED;
    mOwner = nsnull;

    if (mAuthProvider) {
      mAuthProvider->Disconnect(NS_ERROR_ABORT);
      mAuthProvider = nsnull;
    }

    if (mTryConnectTimer) {
      mTryConnectTimer->Cancel();
      mTryConnectTimer = nsnull;
    }

    if (mInitialServerResponseTimer) {
      mInitialServerResponseTimer->Cancel();
      mInitialServerResponseTimer = nsnull;
    }

    if (mProxyResolveCancelable) {
      mProxyResolveCancelable->Cancel(NS_ERROR_ABORT);
      mProxyResolveCancelable = nsnull;
    }

    if (mDNSRequest) {
      mDNSRequest->Cancel(NS_ERROR_ABORT);
      mDNSRequest = nsnull;
    }

    if (mSocketInput) {
      mSocketInput->Close();
      mSocketInput = nsnull;
    }
    if (mSocketOutput) {
      mSocketOutput->Close();
      mSocketOutput = nsnull;
    }
    if (mSocketTransport) {
      mSocketTransport->Close(NS_OK);
      mSocketTransport = nsnull;
    }
    mProxyInfo = nsnull;

    while (mOutgoingMessages.GetSize() != 0) {
      delete static_cast<nsCString*>(mOutgoingMessages.PopFront());
    }

    while (mReceivedMessages.GetSize() != 0) {
      delete static_cast<nsString*>(mReceivedMessages.PopFront());
    }

    
    
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsWebSocketEstablishedConnection::
                                  RemoveFromLoadGroup);
    if (event) {
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    }

    nsLayoutStatics::Release();
  }

  return NS_OK;
}

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(Retry)
{
  nsresult rv;

  for (PRUint32 i = 0; i < kHeadersLen; ++i) {
    mHeaders[i].Truncate();
  }

  rv = OnProxyAvailable(nsnull, mOwner->mURI, mProxyInfo, NS_OK);
  CHECK_SUCCESS_AND_FAIL_IF_FAILED(rv);
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(ResolveNextProxyAndConnect)
{
  nsresult rv;

  if (mCurrentProxyConfig == eResolvingProxyFailed) {
    return;
  }

  nsCOMPtr<nsIProtocolProxyService2> proxyService =
    do_GetService(NS_PROTOCOLPROXYSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed getting proxyService");
    mCurrentProxyConfig = eResolvingProxyFailed;
    mFailureStatus = NS_ERROR_UNKNOWN_PROXY_HOST;
    FailConnection();
    return;
  }

  if (mProxyInfo) {
    
    
    

    {
      MutexAutoLock lockDisconnect(mLockDisconnect);
      rv = Reset();
      CHECK_SUCCESS_AND_FAIL_IF_FAILED(rv);
    }

    nsCOMPtr<nsIProxyInfo> pi;
    rv = proxyService->GetFailoverForProxy(mProxyInfo, mOwner->mURI,
                                           mProxyFailureReason,
                                           getter_AddRefs(pi));
    if (NS_FAILED(rv)) {
      mProxyInfo = nsnull;
      ResolveNextProxyAndConnect();
      return;
    }

    OnProxyAvailable(nsnull, mOwner->mURI, pi, NS_OK);
    return;
  }

  

  PRUint32 flags = nsIProtocolProxyService::RESOLVE_IGNORE_URI_SCHEME;

  if (mCurrentProxyConfig == eNotResolvingProxy) {
    mCurrentProxyConfig = eResolvingSOCKSProxy;
    flags |= nsIProtocolProxyService::RESOLVE_PREFER_SOCKS_PROXY;
  } else if (mCurrentProxyConfig == eResolvingSOCKSProxy) {
    flags |= nsIProtocolProxyService::RESOLVE_PREFER_HTTPS_PROXY;
    mCurrentProxyConfig = eResolvingHTTPSProxy;
  } else if (mCurrentProxyConfig == eResolvingHTTPSProxy) {
    mCurrentProxyConfig = eResolvingHTTPProxy;
  } else if (mCurrentProxyConfig == eResolvingHTTPProxy) {
    mCurrentProxyConfig = eResolvingProxyFailed;
    mFailureStatus = NS_ERROR_UNKNOWN_PROXY_HOST;
    FailConnection();
    return;
  }

  rv = proxyService->AsyncResolve(mOwner->mURI,
                                  flags, this,
                                  getter_AddRefs(mProxyResolveCancelable));
  if (NS_FAILED(rv)) {
    ResolveNextProxyAndConnect();
    return;
  }
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END

void
nsWebSocketEstablishedConnection::RemoveFromLoadGroup()
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");
  nsCOMPtr<nsILoadGroup> loadGroup;
  GetLoadGroup(getter_AddRefs(loadGroup));
  if (loadGroup) {
    loadGroup->RemoveRequest(this, nsnull, NS_OK);
  }
}





IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_BEGIN(ProcessAuthentication)
{
  nsresult rv = mAuthProvider->ProcessAuthentication(407, PR_FALSE);

  if (rv == NS_ERROR_IN_PROGRESS) {
    return;
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("ProcessAuthentication failed");
    FailConnection();
    return;
  }

  Retry();
}
IMPL_RUNNABLE_ON_MAIN_THREAD_METHOD_END




#define NOT_IMPLEMENTED_IF_FUNC_BEGIN(_func)                                   \
  NS_IMETHODIMP                                                                \
  nsWebSocketEstablishedConnection::_func

#define NOT_IMPLEMENTED_IF_FUNC_END(_func)                                     \
  {                                                                            \
    return NS_ERROR_NOT_IMPLEMENTED;                                           \
  }

#define NOT_IMPLEMENTED_IF_FUNC_0(_func)                                       \
  NOT_IMPLEMENTED_IF_FUNC_BEGIN(_func) ()                                      \
  NOT_IMPLEMENTED_IF_FUNC_END(_func)

#define NOT_IMPLEMENTED_IF_FUNC_1(_func, _arg)                                 \
  NOT_IMPLEMENTED_IF_FUNC_BEGIN(_func) (_arg)                                  \
  NOT_IMPLEMENTED_IF_FUNC_END(_func)

#define NOT_IMPLEMENTED_IF_FUNC_2(_func, _arg1, arg2)                          \
  NOT_IMPLEMENTED_IF_FUNC_BEGIN(_func) (_arg1, arg2)                           \
  NOT_IMPLEMENTED_IF_FUNC_END(_func)





NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetName(nsACString &aName)
{
  aName = mRequestName;
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::IsPending(PRBool *aValue)
{
  *aValue = !!(mOwner);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetStatus(nsresult *aStatus)
{
  *aStatus = mFailureStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::Cancel(nsresult aStatus)
{
  if (!mOwner) {
    return NS_OK;
  }

  mFailureStatus = aStatus;

  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &nsWebSocketEstablishedConnection::CloseOwner);
  if (event) {
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }

  return NS_OK;
}

NOT_IMPLEMENTED_IF_FUNC_0(Suspend)
NOT_IMPLEMENTED_IF_FUNC_0(Resume)

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
  *aLoadGroup = nsnull;
  if (!mOwner)
    return NS_OK;

  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(mOwner->mScriptContext);

  if (doc) {
    *aLoadGroup = doc->GetDocumentLoadGroup().get();  
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = nsIRequest::LOAD_BACKGROUND;
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  
  return NS_OK;
}





NOT_IMPLEMENTED_IF_FUNC_1(GetOriginalURI, nsIURI **originalURI)
NOT_IMPLEMENTED_IF_FUNC_1(SetOriginalURI, nsIURI *originalURI)
NOT_IMPLEMENTED_IF_FUNC_1(GetOwner, nsISupports **owner)
NOT_IMPLEMENTED_IF_FUNC_1(SetOwner, nsISupports *owner)
NOT_IMPLEMENTED_IF_FUNC_1(SetNotificationCallbacks,
                          nsIInterfaceRequestor *callbacks)
NOT_IMPLEMENTED_IF_FUNC_1(GetSecurityInfo, nsISupports **securityInfo)
NOT_IMPLEMENTED_IF_FUNC_1(GetContentType, nsACString &value)
NOT_IMPLEMENTED_IF_FUNC_1(SetContentType, const nsACString &value)
NOT_IMPLEMENTED_IF_FUNC_1(GetContentCharset, nsACString &value)
NOT_IMPLEMENTED_IF_FUNC_1(SetContentCharset, const nsACString &value)
NOT_IMPLEMENTED_IF_FUNC_1(GetContentLength, PRInt32 *value)
NOT_IMPLEMENTED_IF_FUNC_1(SetContentLength, PRInt32 value)
NOT_IMPLEMENTED_IF_FUNC_1(Open, nsIInputStream **_retval)
NOT_IMPLEMENTED_IF_FUNC_2(AsyncOpen, nsIStreamListener *listener,
                          nsISupports *context)





NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetProxyInfo(nsIProxyInfo **result)
{
  NS_IF_ADDREF(*result = mProxyInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetIsSSL(PRBool *aIsSSL)
{
  *aIsSSL = mOwner->mSecure;
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetProxyMethodIsConnect(PRBool *aProxyMethodIsConnect)
{
  *aProxyMethodIsConnect = UsingHttpProxy();
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetURI(nsIURI **aURI)
{
  NS_IF_ADDREF(*aURI = mOwner->mURI);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetNotificationCallbacks(nsIInterfaceRequestor **callbacks)
{
  NS_ADDREF(*callbacks = this);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetRequestMethod(nsACString &method)
{
  if (mAuthenticating) {
    method.AssignLiteral("CONNECT");
  } else {
    method.AssignLiteral("GET");
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetServerResponseHeader(nsACString &value)
{
  if (mHeaders[kServerPos].IsEmpty()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  value.Assign(mHeaders[kServerPos]);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetProxyChallenges(nsACString &value)
{
  if (mHeaders[kProxyAuthenticatePos].IsEmpty()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  value.Assign(mHeaders[kProxyAuthenticatePos]);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetWWWChallenges(nsACString &value)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::SetProxyCredentials(const nsACString &value)
{
  mProxyCredentials.Assign(value);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::SetWWWCredentials(const nsACString &value)
{
  mCredentials.Assign(value);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnAuthAvailable()
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  if (!mOwner) {
    return NS_OK;
  }

  return Retry();
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnAuthCancelled(PRBool userCancel)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  if (!mOwner) {
    return NS_OK;
  }

  if (!userCancel) {
    return FailConnection();
  }

  return CloseOwner();
}





NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnLookupComplete(nsICancelable *aRequest,
                                                   nsIDNSRecord  *aRec,
                                                   nsresult       aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  mDNSRequest = nsnull;
  mFailureStatus = aStatus;
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(aStatus, aStatus);

  nsresult rv;

  rv = aRec->GetNextAddr(mOwner->mPort, &mPRNetAddr);
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  TryConnect(nsnull, this);

  return NS_OK;
}





NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnProxyAvailable(nsICancelable *aRequest,
                                                   nsIURI *aUri,
                                                   nsIProxyInfo *aProxyInfo,
                                                   nsresult aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  nsresult rv;

  if (!mOwner) {
    return NS_ERROR_ABORT;
  }

  if (NS_FAILED(aStatus)) {
    return ResolveNextProxyAndConnect();
  }

  mProxyInfo = aProxyInfo;

  if (mProxyInfo) {
    TryConnect(nsnull, this);
  } else {
    
    

    nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID, &rv);
    ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

    nsCOMPtr<nsIThread> thread = do_GetMainThread();
    rv = dns->AsyncResolve(mOwner->mAsciiHost,
                           0, this, thread, getter_AddRefs(mDNSRequest));
    ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);
  }

  return NS_OK;
}





nsresult
nsWebSocketEstablishedConnection::OnInputStreamReady(nsIAsyncInputStream *aStream)
{
  NS_ASSERTION(!NS_IsMainThread(), "Not running on socket thread");

  nsresult rv;

  {
    MutexAutoLock lockDisconnect(mLockDisconnect);

    if (!mOwner) {
      return NS_ERROR_ABORT;
    }

    NS_ASSERTION(aStream == mSocketInput, "unexpected stream");

    while (PR_TRUE) {
      if (mBuffer.Length() - mBytesInBuffer < DEFAULT_BUFFER_SIZE) {
        PRUint32 newLen = mBuffer.Length() + DEFAULT_BUFFER_SIZE;
        mBuffer.SetLength(newLen);
        ENSURE_TRUE_AND_FAIL_IF_FAILED(mBuffer.Length() == newLen,
                                       NS_ERROR_OUT_OF_MEMORY);
      }

      PRUint32 read;
      rv = aStream->Read(mBuffer.BeginWriting() + mBytesInBuffer,
                         DEFAULT_BUFFER_SIZE, &read);
      if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
        break;
      }
      mFailureStatus = rv;
      ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

      
      if (read == 0) {
        
        
        if (mStatus != CONN_RETRYING_TO_AUTHENTICATE) {
          CloseOwner();
        }
        mFailureStatus = NS_BASE_STREAM_CLOSED;
        return NS_BASE_STREAM_CLOSED;
      }

      PRUint32 start = mBytesInBuffer;
      mBytesInBuffer += read;
      rv = HandleNewInputString(start);
      ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);
    }

    rv = mSocketInput->AsyncWait(this, 0, 0, gWebSocketThread);
    ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);
  }

  return NS_OK;
}





nsresult
nsWebSocketEstablishedConnection::OnOutputStreamReady(nsIAsyncOutputStream *aStream)
{
  NS_ASSERTION(!NS_IsMainThread(), "Not running on socket thread");

  nsresult rv;

  {
    MutexAutoLock lockDisconnect(mLockDisconnect);

    if (!mOwner) {
      return NS_ERROR_ABORT;
    }

    NS_ASSERTION(aStream == mSocketOutput, "unexpected stream");

    {
      MutexAutoLock lockOut(mLockOutgoingMessages);

      while (PR_TRUE) {
        if (mOutgoingMessages.GetSize() == 0) {
          break;
        }

        

        nsCString *strToSend =
          static_cast<nsCString*>(mOutgoingMessages.PeekFront());
        PRUint32 sizeToSend =
          strToSend->Length() - mBytesAlreadySentOfFirstOutString;
        PRBool currentStrHasStartFrameByte =
          (mBytesAlreadySentOfFirstOutString == 0);
        PRBool strIsMessage = (mStatus >= CONN_CONNECTED_AND_READY);

        if (sizeToSend != 0) {
          PRUint32 written;
          rv = aStream->Write(strToSend->get() + mBytesAlreadySentOfFirstOutString,
                              sizeToSend, &written);
          if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
            break;
          }

          
          if ((mStatus == CONN_CONNECTING_TO_HTTP_PROXY ||
               (mStatus == CONN_SENDING_INITIAL_REQUEST && mProxyInfo)) &&
              (rv == NS_ERROR_PROXY_CONNECTION_REFUSED ||
               rv == NS_ERROR_UNKNOWN_PROXY_HOST ||
               rv == NS_ERROR_NET_TIMEOUT ||
               rv == NS_ERROR_NET_RESET)) {
            mProxyFailureReason = rv;
            return ResolveNextProxyAndConnect();
          }

          mFailureStatus = rv;
          ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

          if (written == 0) {
            CloseOwner();
            mFailureStatus = NS_BASE_STREAM_CLOSED;
            return NS_BASE_STREAM_CLOSED;
          }

          if (strIsMessage) {
            PRBool currentStrHasEndFrameByte =
              (mBytesAlreadySentOfFirstOutString + written ==
               strToSend->Length());

            
            if (currentStrHasStartFrameByte) {
              if (currentStrHasEndFrameByte) {
                mOutgoingBufferedAmount -= written - 2;
              } else {
                mOutgoingBufferedAmount -= written - 1;
              }
            } else {
              if (currentStrHasEndFrameByte) {
                mOutgoingBufferedAmount -= written - 1;
              } else {
                mOutgoingBufferedAmount -= written;
              }
            }
          }

          mBytesAlreadySentOfFirstOutString += written;
        }

        sizeToSend = strToSend->Length() - mBytesAlreadySentOfFirstOutString;
        if (sizeToSend != 0) { 
          break;
        }

        
        mOutgoingMessages.PopFront();
        delete strToSend;
        mBytesAlreadySentOfFirstOutString = 0;
      }

      if (mOutgoingMessages.GetSize() != 0) {
        rv = mSocketOutput->AsyncWait(this, 0, 0, gWebSocketThread);
        ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);
      } else {
        if (mStatus == CONN_SENDING_INITIAL_REQUEST ||
            mStatus == CONN_CONNECTING_TO_HTTP_PROXY) {
          if (mStatus == CONN_SENDING_INITIAL_REQUEST) {
            mStatus = CONN_WAITING_RESPONSE_FOR_INITIAL_REQUEST;

            rv = mInitialServerResponseTimer->
              InitWithFuncCallback(TimerInitialServerResponseCallback, this,
                                   TIMEOUT_WAIT_FOR_SERVER_RESPONSE,
                                   nsITimer::TYPE_ONE_SHOT);
            ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);
          }
          rv = mSocketInput->AsyncWait(this, 0, 0, gWebSocketThread);
          ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);
        }
      }
    }
  }

  return NS_OK;
}





NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetInterface(const nsIID &aIID,
                                               void **aResult)
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on main thread");

  if (aIID.Equals(NS_GET_IID(nsIAuthPrompt)) ||
      aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {
    nsresult rv;

    nsCOMPtr<nsIDocument> doc =
      nsContentUtils::GetDocumentFromScriptContext(mOwner->mScriptContext);

    if (!doc) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    nsCOMPtr<nsIPromptFactory> wwatch =
      do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsPIDOMWindow> outerWindow = doc->GetWindow();
    return wwatch->GetPrompt(outerWindow, aIID, aResult);
  }

  return NS_ERROR_UNEXPECTED;
}





nsWebSocket::nsWebSocket() : mReadyState(nsIWebSocket::CONNECTING),
                             mOutgoingBufferedAmount(0)
{
}

nsWebSocket::~nsWebSocket()
{
  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nsnull;
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsWebSocket)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsWebSocket,
                                                  nsDOMEventTargetWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnOpenListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnMessageListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnCloseListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPrincipal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mURI)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mConnection");
  cb.NoteXPCOMChild(static_cast<nsIInterfaceRequestor*>(tmp->mConnection));
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsWebSocket,
                                                nsDOMEventTargetWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnOpenListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnMessageListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnCloseListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPrincipal)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mURI)
  if (tmp->mConnection) {
    tmp->mConnection->Disconnect();
    tmp->mConnection = nsnull;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

DOMCI_DATA(WebSocket, nsWebSocket)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsWebSocket)
  NS_INTERFACE_MAP_ENTRY(nsIWebSocket)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebSocket)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetWrapperCache)

NS_IMPL_ADDREF_INHERITED(nsWebSocket, nsDOMEventTargetWrapperCache)
NS_IMPL_RELEASE_INHERITED(nsWebSocket, nsDOMEventTargetWrapperCache)











NS_IMETHODIMP
nsWebSocket::Initialize(nsISupports* aOwner,
                        JSContext* cx,
                        JSObject* obj,
                        PRUint32 argc,
                        jsval* argv)
{
  nsAutoString urlParam, protocolParam;

  PRBool prefEnabled =
    nsContentUtils::GetBoolPref("network.websocket.enabled", PR_TRUE);
  if (!prefEnabled) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (argc != 1 && argc != 2) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  JSAutoRequest ar(cx);

  JSString* jsstr = JS_ValueToString(cx, argv[0]);
  if (!jsstr) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }
  urlParam.Assign(reinterpret_cast<const PRUnichar*>(JS_GetStringChars(jsstr)),
                  JS_GetStringLength(jsstr));

  if (argc == 2) {
    jsstr = JS_ValueToString(cx, argv[1]);
    if (!jsstr) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    protocolParam.
      Assign(reinterpret_cast<const PRUnichar*>(JS_GetStringChars(jsstr)),
             JS_GetStringLength(jsstr));
  }

  nsCOMPtr<nsPIDOMWindow> ownerWindow = do_QueryInterface(aOwner);
  NS_ENSURE_STATE(ownerWindow);

  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aOwner);
  NS_ENSURE_STATE(sgo);
  nsCOMPtr<nsIScriptContext> scriptContext = sgo->GetContext();
  NS_ENSURE_STATE(scriptContext);

  nsCOMPtr<nsIScriptObjectPrincipal> scriptPrincipal(do_QueryInterface(aOwner));
  NS_ENSURE_STATE(scriptPrincipal);
  nsCOMPtr<nsIPrincipal> principal = scriptPrincipal->GetPrincipal();
  NS_ENSURE_STATE(principal);

  return Init(principal, scriptContext, ownerWindow, urlParam, protocolParam);
}





nsresult
nsWebSocket::EstablishConnection()
{
  NS_ASSERTION(!mConnection, "mConnection should be null");

  nsresult rv;

  nsRefPtr<nsWebSocketEstablishedConnection> conn =
    new nsWebSocketEstablishedConnection();
  NS_ENSURE_TRUE(conn, NS_ERROR_OUT_OF_MEMORY);

  rv = conn->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  mConnection = conn;

  return NS_OK;
}

nsresult
nsWebSocket::SetReadyState(PRInt32 aNewReadyState)
{
  if (mReadyState == aNewReadyState) {
    return NS_OK;
  }

  NS_ENSURE_TRUE((aNewReadyState == nsIWebSocket::OPEN) ||
                 (aNewReadyState == nsIWebSocket::CLOSED),
                 NS_ERROR_UNEXPECTED);

  mReadyState = aNewReadyState;

  nsresult rv;

  nsCOMPtr<nsIDocument> doc =
     nsContentUtils::GetDocumentFromScriptContext(mScriptContext);
  rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv) || !doc) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMEvent(getter_AddRefs(event), nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  
  if (mReadyState == nsIWebSocket::OPEN) {
    rv = event->InitEvent(NS_LITERAL_STRING("open"), PR_FALSE, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mReadyState == nsIWebSocket::CLOSED) {
    rv = event->InitEvent(NS_LITERAL_STRING("close"), PR_FALSE, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, NS_OK);
  }

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  rv = privateEvent->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  rv = DispatchDOMEvent(nsnull, event, nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsWebSocket::ParseURL(const nsString& aURL)
{
  nsresult rv;

  NS_ENSURE_TRUE(!aURL.IsEmpty(), NS_ERROR_DOM_SYNTAX_ERR);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

  nsCOMPtr<nsIURL> parsedURL(do_QueryInterface(uri, &rv));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

  nsCAutoString fragment;
  rv = parsedURL->GetRef(fragment);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && fragment.IsEmpty(),
                 NS_ERROR_DOM_SYNTAX_ERR);

  nsCAutoString scheme;
  rv = parsedURL->GetScheme(scheme);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && !scheme.IsEmpty(),
                 NS_ERROR_DOM_SYNTAX_ERR);

  nsCAutoString host;
  rv = parsedURL->GetAsciiHost(host);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && !host.IsEmpty(), NS_ERROR_DOM_SYNTAX_ERR);

  PRInt32 port;
  rv = parsedURL->GetPort(&port);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

  rv = NS_CheckPortSafety(port, scheme.get());
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

  nsCAutoString filePath;
  rv = parsedURL->GetFilePath(filePath);
  if (filePath.IsEmpty()) {
    filePath.AssignLiteral("/");
  }
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

  nsCAutoString query;
  rv = parsedURL->GetQuery(query);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

  nsXPIDLCString origin;
  rv = mPrincipal->GetOrigin(getter_Copies(origin));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

  if (scheme.LowerCaseEqualsLiteral("ws")) {
     mSecure = PR_FALSE;
     mPort = (port == -1) ? DEFAULT_WS_SCHEME_PORT : port;
  } else if (scheme.LowerCaseEqualsLiteral("wss")) {
    mSecure = PR_TRUE;
    mPort = (port == -1) ? DEFAULT_WSS_SCHEME_PORT : port;
  } else {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  mOrigin = origin;
  ToLowerCase(mOrigin);

  mAsciiHost = host;
  ToLowerCase(mAsciiHost);

  mResource = filePath;
  if (!query.IsEmpty()) {
    mResource.AppendLiteral("?");
    mResource.Append(query);
  }
  PRUint32 length = mResource.Length();
  PRUint32 i;
  for (i = 0; i < length; ++i) {
    if (mResource[i] < static_cast<PRUnichar>(0x0021) ||
        mResource[i] > static_cast<PRUnichar>(0x007E)) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
  }

  mOriginalURL = aURL;

  mURI = parsedURL;

  return NS_OK;
}

nsresult
nsWebSocket::SetProtocol(const nsString& aProtocol)
{
  if (aProtocol.IsEmpty()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  PRUint32 length = aProtocol.Length();
  PRUint32 i;
  for (i = 0; i < length; ++i) {
    if (aProtocol[i] < static_cast<PRUnichar>(0x0020) ||
        aProtocol[i] > static_cast<PRUnichar>(0x007E)) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
  }

  CopyUTF16toUTF8(aProtocol, mProtocol);
  return NS_OK;
}





NS_IMETHODIMP
nsWebSocket::GetURL(nsAString& aURL)
{
  aURL = mOriginalURL;
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocket::GetReadyState(PRInt32 *aReadyState)
{
  *aReadyState = mReadyState;
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocket::GetBufferedAmount(PRUint32 *aBufferedAmount)
{
  if (!mConnection) {
    *aBufferedAmount = mOutgoingBufferedAmount;
  } else {
    *aBufferedAmount = mConnection->GetOutgoingBufferedAmount();
  }
  return NS_OK;
}

#define NS_WEBSOCKET_IMPL_DOMEVENTLISTENER(_eventlistenername, _eventlistener) \
  NS_IMETHODIMP                                                                \
  nsWebSocket::GetOn##_eventlistenername(nsIDOMEventListener * *aEventListener)\
  {                                                                            \
    return GetInnerEventListener(_eventlistener, aEventListener);              \
  }                                                                            \
                                                                               \
  NS_IMETHODIMP                                                                \
  nsWebSocket::SetOn##_eventlistenername(nsIDOMEventListener * aEventListener) \
  {                                                                            \
    return RemoveAddEventListener(NS_LITERAL_STRING(#_eventlistenername),      \
                                  _eventlistener, aEventListener);             \
  }

NS_WEBSOCKET_IMPL_DOMEVENTLISTENER(open, mOnOpenListener)
NS_WEBSOCKET_IMPL_DOMEVENTLISTENER(message, mOnMessageListener)
NS_WEBSOCKET_IMPL_DOMEVENTLISTENER(close, mOnCloseListener)

NS_IMETHODIMP
nsWebSocket::Send(const nsAString& aData, PRBool *aRet)
{
  *aRet = PR_FALSE;

  if (mReadyState == nsIWebSocket::CONNECTING) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  
  PRUint32 i, length = aData.Length();
  for (i = 0; i < length; ++i) {
    if (NS_IS_LOW_SURROGATE(aData[i])) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }
    if (NS_IS_HIGH_SURROGATE(aData[i])) {
      if (i + 1 == length || !NS_IS_LOW_SURROGATE(aData[i + 1])) {
        return NS_ERROR_DOM_SYNTAX_ERR;
      }
      ++i;
      continue;
    }
  }

  if (mReadyState == nsIWebSocket::CLOSED) {
    mOutgoingBufferedAmount += NS_ConvertUTF16toUTF8(aData).Length();
    return NS_OK;
  }

  nsresult rv = mConnection->PostMessage(PromiseFlatString(aData));
  *aRet = NS_SUCCEEDED(rv);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocket::Close()
{
  if (mReadyState == nsIWebSocket::CLOSED) {
    return NS_OK;
  }

  if (mConnection) {
    mOutgoingBufferedAmount = mConnection->GetOutgoingBufferedAmount();
    mConnection->Disconnect();
    mConnection = nsnull;
  }

  SetReadyState(nsIWebSocket::CLOSED);

  return NS_OK;
}




NS_IMETHODIMP
nsWebSocket::Init(nsIPrincipal* aPrincipal,
                  nsIScriptContext* aScriptContext,
                  nsPIDOMWindow* aOwnerWindow,
                  const nsAString& aURL,
                  const nsAString& aProtocol)
{
  nsresult rv;

  NS_ENSURE_ARG(aPrincipal);

  PRBool prefEnabled =
    nsContentUtils::GetBoolPref("network.websocket.enabled", PR_TRUE);
  if (!prefEnabled) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  mPrincipal = aPrincipal;
  mScriptContext = aScriptContext;
  if (aOwnerWindow) {
    mOwner = aOwnerWindow->GetCurrentInnerWindow();
  }
  else {
    mOwner = nsnull;
  }

  
  rv = ParseURL(PromiseFlatString(aURL));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!aProtocol.IsEmpty()) {
    rv = SetProtocol(PromiseFlatString(aProtocol));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  EstablishConnection();

  return NS_OK;
}


void
nsWebSocket::ReleaseGlobals()
{
  if (nsWebSocketEstablishedConnection::sWSsConnecting) {
    nsWebSocketEstablishedConnection::sWSsConnecting->Clear();
    delete nsWebSocketEstablishedConnection::sWSsConnecting;
    nsWebSocketEstablishedConnection::sWSsConnecting = nsnull;
  }
  if (gWebSocketThread) {
    gWebSocketThread->Shutdown();
    NS_RELEASE(gWebSocketThread);
  }
}





NS_IMPL_ISUPPORTS2(nsWSProtocolHandler,
                   nsIProtocolHandler, nsIProxiedProtocolHandler)

NS_IMETHODIMP
nsWSProtocolHandler::GetScheme(nsACString& aScheme)
{
  aScheme.AssignLiteral("ws");
  return NS_OK;
}

NS_IMETHODIMP
nsWSProtocolHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
  *aDefaultPort = DEFAULT_WS_SCHEME_PORT;
  return NS_OK;
}

NS_IMETHODIMP
nsWSProtocolHandler::GetProtocolFlags(PRUint32 *aProtocolFlags)
{
  *aProtocolFlags = URI_STD | URI_NON_PERSISTABLE | URI_DOES_NOT_RETURN_DATA |
                    ALLOWS_PROXY | ALLOWS_PROXY_HTTP | URI_DANGEROUS_TO_LOAD;
  return NS_OK;
}

NS_IMETHODIMP
nsWSProtocolHandler::NewURI(const nsACString& aSpec,
                            const char *aCharset,
                            nsIURI *aBaseURI,
                            nsIURI **aURI)
{
  nsresult rv;
  nsCOMPtr<nsIStandardURL> url(do_CreateInstance(NS_STANDARDURL_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 defaultPort;
  GetDefaultPort(&defaultPort);

  rv = url->Init(nsIStandardURL::URLTYPE_AUTHORITY,
                 defaultPort, aSpec, aCharset, aBaseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallQueryInterface(url, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsWSProtocolHandler::NewChannel(nsIURI *aURI,
                                nsIChannel **aChannel)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsWSProtocolHandler::NewProxiedChannel(nsIURI *aURI,
                                       nsIProxyInfo* aProxyInfo,
                                       nsIChannel **aChannel)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsWSProtocolHandler::AllowPort(PRInt32 aPort,
                               const char *aScheme,
                               PRBool *aAllowPort)
{
  PRInt32 defaultPort;
  GetDefaultPort(&defaultPort);

  *aAllowPort = (aPort == defaultPort);
  return NS_OK;
}





NS_IMETHODIMP
nsWSSProtocolHandler::GetScheme(nsACString& aScheme)
{
  aScheme.AssignLiteral("wss");
  return NS_OK;
}

NS_IMETHODIMP
nsWSSProtocolHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
  *aDefaultPort = DEFAULT_WSS_SCHEME_PORT;
  return NS_OK;
}
