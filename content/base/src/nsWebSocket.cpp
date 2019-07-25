







































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
#include "nsDOMClassInfoID.h"
#include "jsapi.h"
#include "nsIURL.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIInterfaceRequestor.h"
#include "nsICharsetConverterManager.h"
#include "nsIUnicodeEncoder.h"
#include "nsThreadUtils.h"
#include "nsIDOMMessageEvent.h"
#include "nsIPromptFactory.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsIStringBundle.h"
#include "nsIConsoleService.h"
#include "nsLayoutStatics.h"
#include "nsIDOMCloseEvent.h"
#include "nsICryptoHash.h"
#include "jsdbgapi.h"
#include "nsIJSContextStack.h"
#include "nsJSUtils.h"
#include "nsIScriptError.h"
#include "nsNetUtil.h"
#include "nsIWebSocketChannel.h"
#include "nsIWebSocketListener.h"
#include "nsILoadGroup.h"
#include "nsIRequest.h"
#include "mozilla/Preferences.h"
#include "nsDOMLists.h"
#include "xpcpublic.h"

using namespace mozilla;





#define UTF_8_REPLACEMENT_CHAR    static_cast<PRUnichar>(0xFFFD)

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


class nsWebSocketEstablishedConnection: public nsIInterfaceRequestor,
                                        public nsIWebSocketListener,
                                        public nsIRequest
{
public:
  nsWebSocketEstablishedConnection();
  virtual ~nsWebSocketEstablishedConnection();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIWEBSOCKETLISTENER
  NS_DECL_NSIREQUEST

  nsresult Init(nsWebSocket *aOwner);
  nsresult Disconnect();

  
  
  nsresult Close();
  nsresult FailConnection();
  nsresult ConsoleError();

  bool HasOutgoingMessages()
  { return mOutgoingBufferedAmount != 0; }

  bool ClosedCleanly() { return mClosedCleanly; }

  nsresult PostMessage(const nsString& aMessage);
  PRUint32 GetOutgoingBufferedAmount() { return mOutgoingBufferedAmount; }

private:

  nsresult PrintErrorOnConsole(const char       *aBundleURI,
                               const PRUnichar  *aError,
                               const PRUnichar **aFormatStrings,
                               PRUint32          aFormatStringsLen);
  nsresult UpdateMustKeepAlive();
  
  
  PRUint32 mOutgoingBufferedAmount;

  nsWebSocket* mOwner; 
  nsCOMPtr<nsIWebSocketChannel> mWebSocketChannel;

  bool mClosedCleanly;

  enum ConnectionStatus {
    CONN_NOT_CONNECTED,
    CONN_CONNECTED_AND_READY,
    CONN_CLOSED
  };

  ConnectionStatus mStatus;
};





NS_IMPL_THREADSAFE_ISUPPORTS3(nsWebSocketEstablishedConnection,
                              nsIInterfaceRequestor,
                              nsIWebSocketListener,
                              nsIRequest)





nsWebSocketEstablishedConnection::nsWebSocketEstablishedConnection() :
  mOutgoingBufferedAmount(0),
  mOwner(nsnull),
  mClosedCleanly(PR_FALSE),
  mStatus(CONN_NOT_CONNECTED)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsLayoutStatics::AddRef();
}

nsWebSocketEstablishedConnection::~nsWebSocketEstablishedConnection()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  NS_ABORT_IF_FALSE(!mOwner, "Disconnect wasn't called!");
  NS_ABORT_IF_FALSE(!mWebSocketChannel, "Disconnect wasn't called!");
}

nsresult
nsWebSocketEstablishedConnection::PostMessage(const nsString& aMessage)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  if (!mOwner) {
    return NS_OK;
  }

  
  NS_ENSURE_STATE(mStatus >= CONN_CONNECTED_AND_READY);

  nsresult rv;

  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  nsCOMPtr<nsIUnicodeEncoder> converter;
  rv = ccm->GetUnicodeEncoder("UTF-8", getter_AddRefs(converter));
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  rv = converter->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace,
                                         nsnull, UTF_8_REPLACEMENT_CHAR);
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  PRInt32 inLen = aMessage.Length();
  PRInt32 maxLen;
  rv = converter->GetMaxLength(aMessage.BeginReading(), inLen, &maxLen);
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  nsCString buf;
  buf.SetLength(maxLen);
  ENSURE_TRUE_AND_FAIL_IF_FAILED(buf.Length() == static_cast<PRUint32>(maxLen),
                                 NS_ERROR_OUT_OF_MEMORY);

  char* start = buf.BeginWriting();

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

  buf.SetLength(outLen);
  ENSURE_TRUE_AND_FAIL_IF_FAILED(buf.Length() == static_cast<PRUint32>(outLen),
                                 NS_ERROR_UNEXPECTED);

  if (mStatus == CONN_CLOSED) {
    NS_ABORT_IF_FALSE(mOwner, "Posting data after disconnecting the websocket");
    
    
    rv = NS_BASE_STREAM_CLOSED;
  } else {
    mOutgoingBufferedAmount += buf.Length();
    mWebSocketChannel->SendMsg(buf);
    rv = NS_OK;
  }

  UpdateMustKeepAlive();
  ENSURE_SUCCESS_AND_FAIL_IF_FAILED(rv, rv);

  return NS_OK;
}

nsresult
nsWebSocketEstablishedConnection::Init(nsWebSocket *aOwner)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  NS_ABORT_IF_FALSE(!mOwner, "WebSocket's connection is already initialized");

  nsresult rv;
  mOwner = aOwner;

  if (mOwner->mSecure) {
    mWebSocketChannel =
      do_CreateInstance("@mozilla.org/network/protocol;1?name=wss", &rv);
  }
  else {
    mWebSocketChannel =
      do_CreateInstance("@mozilla.org/network/protocol;1?name=ws", &rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mWebSocketChannel->SetNotificationCallbacks(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  rv = GetLoadGroup(getter_AddRefs(loadGroup));
  if (loadGroup) {
    rv = mWebSocketChannel->SetLoadGroup(loadGroup);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = loadGroup->AddRequest(this, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mOwner->mRequestedProtocolList.IsEmpty()) {
    rv = mWebSocketChannel->SetProtocol(mOwner->mRequestedProtocolList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCString asciiOrigin;
  rv = nsContentUtils::GetASCIIOrigin(mOwner->mPrincipal, asciiOrigin);
  NS_ENSURE_SUCCESS(rv, rv);

  ToLowerCase(asciiOrigin);

  rv = mWebSocketChannel->AsyncOpen(mOwner->mURI,
                                    asciiOrigin, this, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsWebSocketEstablishedConnection::PrintErrorOnConsole(const char *aBundleURI,
                                                      const PRUnichar *aError,
                                                      const PRUnichar **aFormatStrings,
                                                      PRUint32 aFormatStringsLen)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  NS_ABORT_IF_FALSE(mOwner, "No owner");

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

  nsCOMPtr<nsIScriptError2> errorObject(
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv));
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

  errorObject->InitWithWindowID
    (message.get(),
     NS_ConvertUTF8toUTF16(mOwner->GetScriptFile()).get(),
     nsnull,
     mOwner->GetScriptLine(), 0, nsIScriptError::errorFlag,
     "Web Socket", mOwner->InnerWindowID()
     );
  
  
  nsCOMPtr<nsIScriptError> logError(do_QueryInterface(errorObject));
  rv = console->LogMessage(logError);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsWebSocketEstablishedConnection::Close()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  if (!mOwner)
    return NS_OK;

  
  
  nsRefPtr<nsWebSocketEstablishedConnection> kungfuDeathGrip = this;

  if (mOwner->mReadyState == nsIMozWebSocket::CONNECTING) {
    mOwner->SetReadyState(nsIMozWebSocket::CLOSED);
    mWebSocketChannel->Close(mOwner->mClientReasonCode,
                             mOwner->mClientReason);
    Disconnect();
    return NS_OK;
  }

  mOwner->SetReadyState(nsIMozWebSocket::CLOSING);

  if (mStatus == CONN_CLOSED) {
    mOwner->SetReadyState(nsIMozWebSocket::CLOSED);
    Disconnect();
    return NS_OK;
  }

  return mWebSocketChannel->Close(mOwner->mClientReasonCode,
                                  mOwner->mClientReason);
}

nsresult
nsWebSocketEstablishedConnection::ConsoleError()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv;
  if (!mOwner) return NS_OK;
  
  nsCAutoString targetSpec;
  rv = mOwner->mURI->GetSpec(targetSpec);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to get targetSpec");
  } else {
    NS_ConvertUTF8toUTF16 specUTF16(targetSpec);
    const PRUnichar *formatStrings[] = { specUTF16.get() };
    
    if (mStatus < CONN_CONNECTED_AND_READY) {
      PrintErrorOnConsole("chrome://global/locale/appstrings.properties",
                          NS_LITERAL_STRING("connectionFailure").get(),
                          formatStrings, NS_ARRAY_LENGTH(formatStrings));
    } else {
      PrintErrorOnConsole("chrome://global/locale/appstrings.properties",
                          NS_LITERAL_STRING("netInterrupt").get(),
                          formatStrings, NS_ARRAY_LENGTH(formatStrings));
    }
  }
  
  return rv;
}


nsresult
nsWebSocketEstablishedConnection::FailConnection()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv = ConsoleError();
  Close();
  return rv;
}

nsresult
nsWebSocketEstablishedConnection::Disconnect()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  if (!mOwner) {
    return NS_OK;
  }
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  GetLoadGroup(getter_AddRefs(loadGroup));
  if (loadGroup)
    loadGroup->RemoveRequest(this, nsnull, NS_OK);

  
  
  nsRefPtr<nsWebSocket> kungfuDeathGrip = mOwner;
  
  mOwner->DontKeepAliveAnyMore();
  mStatus = CONN_CLOSED;
  mOwner = nsnull;
  mWebSocketChannel = nsnull;

  nsLayoutStatics::Release();
  return NS_OK;
}

nsresult
nsWebSocketEstablishedConnection::UpdateMustKeepAlive()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  NS_ABORT_IF_FALSE(mOwner, "No owner");

  mOwner->UpdateMustKeepAlive();
  return NS_OK;
}





NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnMessageAvailable(nsISupports *aContext,
                                                     const nsACString & aMsg)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  if (!mOwner)
    return NS_ERROR_NOT_AVAILABLE;
  
  
  nsresult rv = mOwner->CreateAndDispatchMessageEvent(aMsg);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch the message event");
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnBinaryMessageAvailable(
  nsISupports *aContext,
  const nsACString & aMsg)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnStart(nsISupports *aContext)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  if (!mOwner)
    return NS_OK;

  if (!mOwner->mRequestedProtocolList.IsEmpty())
    mWebSocketChannel->GetProtocol(mOwner->mEstablishedProtocol);

  mWebSocketChannel->GetExtensions(mOwner->mEstablishedExtensions);

  mStatus = CONN_CONNECTED_AND_READY;
  mOwner->SetReadyState(nsIMozWebSocket::OPEN);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnStop(nsISupports *aContext,
                                         nsresult aStatusCode)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  if (!mOwner)
    return NS_OK;

  mClosedCleanly = NS_SUCCEEDED(aStatusCode);

  if (aStatusCode == NS_BASE_STREAM_CLOSED && 
      mOwner->mReadyState >= nsIMozWebSocket::CLOSING) {
    
    aStatusCode = NS_OK;
  }

  if (NS_FAILED(aStatusCode)) {
    ConsoleError();
    if (mOwner) {
      nsresult rv =
        mOwner->CreateAndDispatchSimpleEvent(NS_LITERAL_STRING("error"));
      if (NS_FAILED(rv))
        NS_WARNING("Failed to dispatch the error event");
    }
  }

  mStatus = CONN_CLOSED;
  if (mOwner) {
    mOwner->SetReadyState(nsIMozWebSocket::CLOSED);
    Disconnect();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnAcknowledge(nsISupports *aContext,
                                                PRUint32 aSize)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  if (aSize > mOutgoingBufferedAmount)
    return NS_ERROR_UNEXPECTED;
  
  mOutgoingBufferedAmount -= aSize;
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::OnServerClose(nsISupports *aContext,
                                                PRUint16 aCode,
                                                const nsACString &aReason)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  if (mOwner) {
    mOwner->mServerReasonCode = aCode;
    CopyUTF8toUTF16(aReason, mOwner->mServerReason);
  }

  Close();                                        
  return NS_OK;
}





NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetInterface(const nsIID &aIID,
                                               void **aResult)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  if (!mOwner)
    return NS_ERROR_FAILURE;

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





nsWebSocket::nsWebSocket() : mKeepingAlive(PR_FALSE),
                             mCheckMustKeepAlive(PR_TRUE),
                             mTriggeredCloseEvent(PR_FALSE),
                             mClientReasonCode(0),
                             mServerReasonCode(nsIWebSocketChannel::CLOSE_ABNORMAL),
                             mReadyState(nsIMozWebSocket::CONNECTING),
                             mOutgoingBufferedAmount(0),
                             mScriptLine(0),
                             mInnerWindowID(0)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
}

nsWebSocket::~nsWebSocket()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  if (mConnection) {
    mConnection->Disconnect();
    mConnection = nsnull;
  }
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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPrincipal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mURI)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mConnection");
  cb.NoteXPCOMChild(static_cast<nsIInterfaceRequestor*>(tmp->mConnection));
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsWebSocket,
                                                nsDOMEventTargetWrapperCache)
  if (tmp->mConnection) {
    tmp->mConnection->Disconnect();
    tmp->mConnection = nsnull;
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnOpenListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnMessageListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnCloseListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPrincipal)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mURI)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

DOMCI_DATA(MozWebSocket, nsWebSocket)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsWebSocket)
  NS_INTERFACE_MAP_ENTRY(nsIMozWebSocket)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozWebSocket)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetWrapperCache)

NS_IMPL_ADDREF_INHERITED(nsWebSocket, nsDOMEventTargetWrapperCache)
NS_IMPL_RELEASE_INHERITED(nsWebSocket, nsDOMEventTargetWrapperCache)












NS_IMETHODIMP
nsWebSocket::Initialize(nsISupports* aOwner,
                        JSContext* aContext,
                        JSObject* aObject,
                        PRUint32 aArgc,
                        jsval* aArgv)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsAutoString urlParam;

  if (!PrefEnabled()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (aArgc != 1 && aArgc != 2) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  JSAutoRequest ar(aContext);

  JSString* jsstr = JS_ValueToString(aContext, aArgv[0]);
  if (!jsstr) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  JS::Anchor<JSString *> deleteProtector(jsstr);
  size_t length;
  const jschar *chars = JS_GetStringCharsAndLength(aContext, jsstr, &length);
  if (!chars) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  urlParam.Assign(chars, length);
  deleteProtector.clear();

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

  nsTArray<nsString> protocolArray;

  if (aArgc == 2) {
    JSObject *jsobj;

    if (JSVAL_IS_OBJECT(aArgv[1]) &&
        (jsobj = JSVAL_TO_OBJECT(aArgv[1])) &&
        JS_IsArrayObject(aContext, jsobj)) {
      jsuint len;
      JS_GetArrayLength(aContext, jsobj, &len);
      
      for (PRUint32 index = 0; index < len; ++index) {
        jsval value;

        if (!JS_GetElement(aContext, jsobj, index, &value))
          return NS_ERROR_DOM_SYNTAX_ERR;

        jsstr = JS_ValueToString(aContext, value);
        if (!jsstr)
          return NS_ERROR_DOM_SYNTAX_ERR;

        deleteProtector.set(jsstr);
        chars = JS_GetStringCharsAndLength(aContext, jsstr, &length);
        if (!chars)
          return NS_ERROR_OUT_OF_MEMORY;

        nsDependentString protocolElement(chars, length);
        if (protocolElement.IsEmpty())
          return NS_ERROR_DOM_SYNTAX_ERR;
        if (protocolArray.Contains(protocolElement))
          return NS_ERROR_DOM_SYNTAX_ERR;
        if (protocolElement.FindChar(',') != -1)  
          return NS_ERROR_DOM_SYNTAX_ERR;
        protocolArray.AppendElement(protocolElement);
        deleteProtector.clear();
      }
    } else {
      jsstr = JS_ValueToString(aContext, aArgv[1]);
      if (!jsstr)
        return NS_ERROR_DOM_SYNTAX_ERR;
      
      deleteProtector.set(jsstr);
      chars = JS_GetStringCharsAndLength(aContext, jsstr, &length);
      if (!chars)
        return NS_ERROR_OUT_OF_MEMORY;
      
      nsDependentString protocolElement(chars, length);
      if (protocolElement.IsEmpty())
        return NS_ERROR_DOM_SYNTAX_ERR;
      if (protocolElement.FindChar(',') != -1)  
        return NS_ERROR_DOM_SYNTAX_ERR;
      protocolArray.AppendElement(protocolElement);
    }
  }

  return Init(principal, scriptContext, ownerWindow, urlParam, protocolArray);
}





nsresult
nsWebSocket::EstablishConnection()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  NS_ABORT_IF_FALSE(!mConnection, "mConnection should be null");

  nsresult rv;

  nsRefPtr<nsWebSocketEstablishedConnection> conn =
    new nsWebSocketEstablishedConnection();

  rv = conn->Init(this);
  mConnection = conn;
  if (NS_FAILED(rv)) {
    Close(0, EmptyString(), 0);
    mConnection = nsnull;
    return rv;
  }

  return NS_OK;
}

class nsWSCloseEvent : public nsRunnable
{
public:
nsWSCloseEvent(nsWebSocket *aWebSocket, bool aWasClean, 
               PRUint16 aCode, const nsString &aReason)
    : mWebSocket(aWebSocket),
      mWasClean(aWasClean),
      mCode(aCode),
      mReason(aReason)
  {}

  NS_IMETHOD Run()
  {
    nsresult rv = mWebSocket->CreateAndDispatchCloseEvent(mWasClean,
                                                          mCode, mReason);
    mWebSocket->UpdateMustKeepAlive();
    return rv;
  }

private:
  nsRefPtr<nsWebSocket> mWebSocket;
  bool mWasClean;
  PRUint16 mCode;
  nsString mReason;
};

nsresult
nsWebSocket::CreateAndDispatchSimpleEvent(const nsString& aName)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv;

  rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMEvent(getter_AddRefs(event), nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = event->InitEvent(aName, PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  rv = privateEvent->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchDOMEvent(nsnull, event, nsnull, nsnull);
}

nsresult
nsWebSocket::CreateAndDispatchMessageEvent(const nsACString& aData)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv;
  
  rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(mOwner);
  NS_ENSURE_TRUE(sgo, NS_ERROR_FAILURE);

  nsIScriptContext* scriptContext = sgo->GetContext();
  NS_ENSURE_TRUE(scriptContext, NS_ERROR_FAILURE);

  JSContext* cx = scriptContext->GetNativeContext();
  NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);

  

  jsval jsData;
  {
    NS_ConvertUTF8toUTF16 utf16Data(aData);
    JSString* jsString;
    JSAutoRequest ar(cx);
    jsString = JS_NewUCStringCopyN(cx,
                                   utf16Data.get(),
                                   utf16Data.Length());
    NS_ENSURE_TRUE(jsString, NS_ERROR_FAILURE);

    jsData = STRING_TO_JSVAL(jsString);
  }

  
  

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMMessageEvent(getter_AddRefs(event), nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMMessageEvent> messageEvent = do_QueryInterface(event);
  rv = messageEvent->InitMessageEvent(NS_LITERAL_STRING("message"),
                                      PR_FALSE, PR_FALSE,
                                      jsData,
                                      mUTF16Origin,
                                      EmptyString(), nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  rv = privateEvent->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchDOMEvent(nsnull, event, nsnull, nsnull);
}

nsresult
nsWebSocket::CreateAndDispatchCloseEvent(bool aWasClean,
                                         PRUint16 aCode,
                                         const nsString &aReason)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv;

  mTriggeredCloseEvent = PR_TRUE;

  rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  
  

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMCloseEvent(getter_AddRefs(event), nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMCloseEvent> closeEvent = do_QueryInterface(event);
  rv = closeEvent->InitCloseEvent(NS_LITERAL_STRING("close"),
                                  PR_FALSE, PR_FALSE,
                                  aWasClean, aCode, aReason);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  rv = privateEvent->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return DispatchDOMEvent(nsnull, event, nsnull, nsnull);
}

bool
nsWebSocket::PrefEnabled()
{
  return Preferences::GetBool("network.websocket.enabled", true);
}

void
nsWebSocket::SetReadyState(PRUint16 aNewReadyState)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv;

  if (mReadyState == aNewReadyState) {
    return;
  }

  NS_ABORT_IF_FALSE((aNewReadyState == nsIMozWebSocket::OPEN)    ||
                    (aNewReadyState == nsIMozWebSocket::CLOSING) ||
                    (aNewReadyState == nsIMozWebSocket::CLOSED),
                    "unexpected readyState");

  if (aNewReadyState == nsIMozWebSocket::OPEN) {
    NS_ABORT_IF_FALSE(mReadyState == nsIMozWebSocket::CONNECTING,
                      "unexpected readyState transition");
    mReadyState = aNewReadyState;

    rv = CreateAndDispatchSimpleEvent(NS_LITERAL_STRING("open"));
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to dispatch the open event");
    }
    UpdateMustKeepAlive();
    return;
  }

  if (aNewReadyState == nsIMozWebSocket::CLOSING) {
    NS_ABORT_IF_FALSE((mReadyState == nsIMozWebSocket::CONNECTING) ||
                      (mReadyState == nsIMozWebSocket::OPEN),
                      "unexpected readyState transition");
    mReadyState = aNewReadyState;
    return;
  }

  if (aNewReadyState == nsIMozWebSocket::CLOSED) {
    mReadyState = aNewReadyState;

    if (mConnection) {
      
      nsCOMPtr<nsIRunnable> event =
        new nsWSCloseEvent(this,
                           mConnection->ClosedCleanly(),
                           mServerReasonCode,
                           mServerReason);
      mOutgoingBufferedAmount += mConnection->GetOutgoingBufferedAmount();
      mConnection = nsnull; 

      rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to dispatch the close event");
        mTriggeredCloseEvent = PR_TRUE;
        UpdateMustKeepAlive();
      }
    }
  }
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

  if (scheme.LowerCaseEqualsLiteral("ws")) {
     mSecure = PR_FALSE;
     mPort = (port == -1) ? DEFAULT_WS_SCHEME_PORT : port;
  } else if (scheme.LowerCaseEqualsLiteral("wss")) {
    mSecure = PR_TRUE;
    mPort = (port == -1) ? DEFAULT_WSS_SCHEME_PORT : port;
  } else {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  rv = nsContentUtils::GetUTFOrigin(parsedURL, mUTF16Origin);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_SYNTAX_ERR);

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








void
nsWebSocket::UpdateMustKeepAlive()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  if (!mCheckMustKeepAlive) {
    return;
  }

  bool shouldKeepAlive = false;

  if (mListenerManager) {
    switch (mReadyState)
    {
      case nsIMozWebSocket::CONNECTING:
      {
        if (mListenerManager->HasListenersFor(NS_LITERAL_STRING("open")) ||
            mListenerManager->HasListenersFor(NS_LITERAL_STRING("message")) ||
            mListenerManager->HasListenersFor(NS_LITERAL_STRING("close"))) {
          shouldKeepAlive = PR_TRUE;
        }
      }
      break;

      case nsIMozWebSocket::OPEN:
      case nsIMozWebSocket::CLOSING:
      {
        if (mListenerManager->HasListenersFor(NS_LITERAL_STRING("message")) ||
            mListenerManager->HasListenersFor(NS_LITERAL_STRING("close")) ||
            mConnection->HasOutgoingMessages()) {
          shouldKeepAlive = PR_TRUE;
        }
      }
      break;

      case nsIMozWebSocket::CLOSED:
      {
        shouldKeepAlive =
          (!mTriggeredCloseEvent &&
           mListenerManager->HasListenersFor(NS_LITERAL_STRING("close")));
      }
    }
  }

  if (mKeepingAlive && !shouldKeepAlive) {
    mKeepingAlive = PR_FALSE;
    static_cast<nsIDOMEventTarget*>(this)->Release();
  } else if (!mKeepingAlive && shouldKeepAlive) {
    mKeepingAlive = PR_TRUE;
    static_cast<nsIDOMEventTarget*>(this)->AddRef();
  }
}

void
nsWebSocket::DontKeepAliveAnyMore()
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  if (mKeepingAlive) {
    mKeepingAlive = PR_FALSE;
    static_cast<nsIDOMEventTarget*>(this)->Release();
  }
  mCheckMustKeepAlive = PR_FALSE;
}

NS_IMETHODIMP
nsWebSocket::RemoveEventListener(const nsAString& aType,
                                 nsIDOMEventListener* aListener,
                                 bool aUseCapture)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv = nsDOMEventTargetHelper::RemoveEventListener(aType,
                                                            aListener,
                                                            aUseCapture);
  if (NS_SUCCEEDED(rv)) {
    UpdateMustKeepAlive();
  }
  return rv;
}

NS_IMETHODIMP
nsWebSocket::AddEventListener(const nsAString& aType,
                              nsIDOMEventListener *aListener,
                              bool aUseCapture,
                              bool aWantsUntrusted,
                              PRUint8 optional_argc)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv = nsDOMEventTargetHelper::AddEventListener(aType,
                                                         aListener,
                                                         aUseCapture,
                                                         aWantsUntrusted,
                                                         optional_argc);
  if (NS_SUCCEEDED(rv)) {
    UpdateMustKeepAlive();
  }
  return rv;
}





NS_IMETHODIMP
nsWebSocket::GetUrl(nsAString& aURL)
{
  aURL = mOriginalURL;
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocket::GetExtensions(nsAString& aExtensions)
{
  CopyUTF8toUTF16(mEstablishedExtensions, aExtensions);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocket::GetProtocol(nsAString& aProtocol)
{
  CopyUTF8toUTF16(mEstablishedProtocol, aProtocol);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocket::GetReadyState(PRUint16 *aReadyState)
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
NS_WEBSOCKET_IMPL_DOMEVENTLISTENER(error, mOnErrorListener)
NS_WEBSOCKET_IMPL_DOMEVENTLISTENER(message, mOnMessageListener)
NS_WEBSOCKET_IMPL_DOMEVENTLISTENER(close, mOnCloseListener)

static bool
ContainsUnpairedSurrogates(const nsAString& aData)
{
  
  PRUint32 i, length = aData.Length();
  for (i = 0; i < length; ++i) {
    if (NS_IS_LOW_SURROGATE(aData[i])) {
      return PR_TRUE;
    }
    if (NS_IS_HIGH_SURROGATE(aData[i])) {
      ++i;
      if (i == length || !NS_IS_LOW_SURROGATE(aData[i])) {
        return PR_TRUE;
      }
      continue;
    }
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsWebSocket::Send(const nsAString& aData)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  if (mReadyState == nsIMozWebSocket::CONNECTING) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (ContainsUnpairedSurrogates(aData))
    return NS_ERROR_DOM_SYNTAX_ERR;

  if (mReadyState == nsIMozWebSocket::CLOSING ||
      mReadyState == nsIMozWebSocket::CLOSED) {
    mOutgoingBufferedAmount += NS_ConvertUTF16toUTF8(aData).Length();
    return NS_OK;
  }

  mConnection->PostMessage(PromiseFlatString(aData));

  return NS_OK;
}

NS_IMETHODIMP
nsWebSocket::Close(PRUint16 code, const nsAString & reason, PRUint8 argc)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  
  if (argc >= 1) {
    if (code != 1000 && (code < 3000 || code > 4999))
      return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }

  nsCAutoString utf8Reason;
  if (argc >= 2) {
    if (ContainsUnpairedSurrogates(reason))
      return NS_ERROR_DOM_SYNTAX_ERR;

    CopyUTF16toUTF8(reason, utf8Reason);

    
    if (utf8Reason.Length() > 123)
      return NS_ERROR_DOM_SYNTAX_ERR;
  }

  
  if (argc >= 1)
    mClientReasonCode = code;
  if (argc >= 2)
    mClientReason = utf8Reason;
  
  if (mReadyState == nsIMozWebSocket::CLOSING ||
      mReadyState == nsIMozWebSocket::CLOSED) {
    return NS_OK;
  }

  if (mReadyState == nsIMozWebSocket::CONNECTING) {
    
    
    nsRefPtr<nsWebSocket> kungfuDeathGrip = this;

    if (mConnection) {
      mConnection->FailConnection();
    }
    return NS_OK;
  }

  
  mConnection->Close();

  return NS_OK;
}




NS_IMETHODIMP
nsWebSocket::Init(nsIPrincipal* aPrincipal,
                  nsIScriptContext* aScriptContext,
                  nsPIDOMWindow* aOwnerWindow,
                  const nsAString& aURL,
                  nsTArray<nsString> & protocolArray)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv;

  NS_ENSURE_ARG(aPrincipal);

  if (!PrefEnabled()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  mPrincipal = aPrincipal;
  mScriptContext = aScriptContext;
  if (aOwnerWindow) {
    mOwner = aOwnerWindow->IsOuterWindow() ?
      aOwnerWindow->GetCurrentInnerWindow() : aOwnerWindow;
  }
  else {
    mOwner = nsnull;
  }

  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  JSContext* cx = nsnull;
  if (stack && NS_SUCCEEDED(stack->Peek(&cx)) && cx) {
    JSStackFrame *fp = JS_GetScriptedCaller(cx, NULL);
    if (fp) {
      JSScript *script = JS_GetFrameScript(cx, fp);
      if (script) {
        mScriptFile = JS_GetScriptFilename(cx, script);
      }

      jsbytecode *pc = JS_GetFramePC(cx, fp);
      if (script && pc) {
        mScriptLine = JS_PCToLineNumber(cx, script, pc);
      }
    }

    mInnerWindowID = nsJSUtils::GetCurrentlyRunningCodeInnerWindowID(cx);
  }

  
  rv = ParseURL(PromiseFlatString(aURL));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!mSecure && 
      !Preferences::GetBool("network.websocket.allowInsecureFromHTTPS",
                            false)) {
    
    
    
    nsCOMPtr<nsIDocument> originDoc =
      nsContentUtils::GetDocumentFromScriptContext(mScriptContext);
    if (originDoc && originDoc->GetSecurityInfo())
      return NS_ERROR_DOM_SECURITY_ERR;
  }

  
  for (PRUint32 index = 0; index < protocolArray.Length(); ++index) {
    for (PRUint32 i = 0; i < protocolArray[index].Length(); ++i) {
      if (protocolArray[index][i] < static_cast<PRUnichar>(0x0021) ||
          protocolArray[index][i] > static_cast<PRUnichar>(0x007E))
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    if (!mRequestedProtocolList.IsEmpty())
      mRequestedProtocolList.Append(NS_LITERAL_CSTRING(", "));
    AppendUTF16toUTF8(protocolArray[index], mRequestedProtocolList);
  }

  
  
  EstablishConnection();

  return NS_OK;
}





NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetName(nsACString &aName)
{
  if (!mOwner)
    return NS_ERROR_UNEXPECTED;
  
  CopyUTF16toUTF8(mOwner->mOriginalURL, aName);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::IsPending(bool *aValue)
{
  *aValue = !!(mOwner);
  return NS_OK;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::GetStatus(nsresult *aStatus)
{
  *aStatus = NS_OK;
  return NS_OK;
}


NS_IMETHODIMP
nsWebSocketEstablishedConnection::Cancel(nsresult aStatus)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  if (!mOwner) {
    return NS_OK;
  }

  ConsoleError();
  return Close();
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::Suspend()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWebSocketEstablishedConnection::Resume()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

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
