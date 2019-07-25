




































#include "nsXMLHttpRequest.h"
#include "nsISimpleEnumerator.h"
#include "nsIXPConnect.h"
#include "nsICharsetConverterManager.h"
#include "nsLayoutCID.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "nsIUploadChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIDOMSerializer.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsGUIEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "prprf.h"
#include "nsIDOMEventListener.h"
#include "nsIJSContextStack.h"
#include "nsIScriptSecurityManager.h"
#include "nsWeakPtr.h"
#include "nsICharsetAlias.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"
#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "nsIVariant.h"
#include "xpcprivate.h"
#include "nsIParser.h"
#include "nsLoadListenerProxy.h"
#include "nsStringStream.h"
#include "nsIStreamConverterService.h"
#include "nsICachingChannel.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsDOMJSUtils.h"
#include "nsCOMArray.h"
#include "nsDOMClassInfo.h"
#include "nsIScriptableUConv.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsLayoutStatics.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsDOMError.h"
#include "nsIHTMLDocument.h"
#include "nsIMultiPartChannel.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIStorageStream.h"
#include "nsIPromptFactory.h"
#include "nsIWindowWatcher.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsIConsoleService.h"
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "jstypedarray.h"
#include "nsStringBuffer.h"
#include "nsDOMFile.h"
#include "nsIFileChannel.h"

#define LOAD_STR "load"
#define ERROR_STR "error"
#define ABORT_STR "abort"
#define LOADSTART_STR "loadstart"
#define PROGRESS_STR "progress"
#define UPLOADPROGRESS_STR "uploadprogress"
#define READYSTATE_STR "readystatechange"
#define LOADEND_STR "loadend"




#define XML_HTTP_REQUEST_UNSENT           (1 << 0) // 0 UNSENT
#define XML_HTTP_REQUEST_OPENED           (1 << 1) // 1 OPENED
#define XML_HTTP_REQUEST_HEADERS_RECEIVED (1 << 2) // 2 HEADERS_RECEIVED
#define XML_HTTP_REQUEST_LOADING          (1 << 3) // 3 LOADING
#define XML_HTTP_REQUEST_DONE             (1 << 4) // 4 DONE
#define XML_HTTP_REQUEST_SENT             (1 << 5) // Internal, OPENED in IE and external view
#define XML_HTTP_REQUEST_STOPPED          (1 << 6) // Internal, LOADING in IE and external view


#define XML_HTTP_REQUEST_ABORTED        (1 << 7)  // Internal
#define XML_HTTP_REQUEST_ASYNC          (1 << 8)  // Internal
#define XML_HTTP_REQUEST_PARSEBODY      (1 << 9)  // Internal
#define XML_HTTP_REQUEST_SYNCLOOPING    (1 << 10) // Internal
#define XML_HTTP_REQUEST_MULTIPART      (1 << 11) // Internal
#define XML_HTTP_REQUEST_GOT_FINAL_STOP (1 << 12) // Internal
#define XML_HTTP_REQUEST_BACKGROUND     (1 << 13) // Internal


#define XML_HTTP_REQUEST_MPART_HEADERS  (1 << 14) // Internal
#define XML_HTTP_REQUEST_USE_XSITE_AC   (1 << 15) // Internal
#define XML_HTTP_REQUEST_NEED_AC_PREFLIGHT (1 << 16) // Internal
#define XML_HTTP_REQUEST_AC_WITH_CREDENTIALS (1 << 17) // Internal

#define XML_HTTP_REQUEST_LOADSTATES         \
  (XML_HTTP_REQUEST_UNSENT |                \
   XML_HTTP_REQUEST_OPENED |                \
   XML_HTTP_REQUEST_HEADERS_RECEIVED |      \
   XML_HTTP_REQUEST_LOADING |               \
   XML_HTTP_REQUEST_DONE |                  \
   XML_HTTP_REQUEST_SENT |                  \
   XML_HTTP_REQUEST_STOPPED)

#define NS_BADCERTHANDLER_CONTRACTID \
  "@mozilla.org/content/xmlhttprequest-bad-cert-handler;1"

#define NS_PROGRESS_EVENT_INTERVAL 50

class nsResumeTimeoutsEvent : public nsRunnable
{
public:
  nsResumeTimeoutsEvent(nsPIDOMWindow* aWindow) : mWindow(aWindow) {}

  NS_IMETHOD Run()
  {
    mWindow->ResumeTimeouts(PR_FALSE);
    return NS_OK;
  }

private:
  nsCOMPtr<nsPIDOMWindow> mWindow;
};




static void AddLoadFlags(nsIRequest *request, nsLoadFlags newFlags)
{
  nsLoadFlags flags;
  request->GetLoadFlags(&flags);
  flags |= newFlags;
  request->SetLoadFlags(flags);
}

static nsresult IsCapabilityEnabled(const char *capability, PRBool *enabled)
{
  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
  if (!secMan)
    return NS_ERROR_FAILURE;

  return secMan->IsCapabilityEnabled(capability, enabled);
}




class nsMultipartProxyListener : public nsIStreamListener
{
public:
  nsMultipartProxyListener(nsIStreamListener *dest);
  virtual ~nsMultipartProxyListener();

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

private:
  nsCOMPtr<nsIStreamListener> mDestListener;
};


nsMultipartProxyListener::nsMultipartProxyListener(nsIStreamListener *dest)
  : mDestListener(dest)
{
}

nsMultipartProxyListener::~nsMultipartProxyListener()
{
}

NS_IMPL_ISUPPORTS2(nsMultipartProxyListener, nsIStreamListener,
                   nsIRequestObserver)



NS_IMETHODIMP
nsMultipartProxyListener::OnStartRequest(nsIRequest *aRequest,
                                         nsISupports *ctxt)
{
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  nsCAutoString contentType;
  nsresult rv = channel->GetContentType(contentType);

  if (!contentType.EqualsLiteral("multipart/x-mixed-replace")) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  

  nsCOMPtr<nsIXMLHttpRequest> xhr = do_QueryInterface(mDestListener);

  nsCOMPtr<nsIStreamConverterService> convServ =
    do_GetService("@mozilla.org/streamConverters;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIStreamListener> toListener(mDestListener);
    nsCOMPtr<nsIStreamListener> fromListener;

    rv = convServ->AsyncConvertData("multipart/x-mixed-replace",
                                    "*/*",
                                    toListener,
                                    nsnull,
                                    getter_AddRefs(fromListener));
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && fromListener, NS_ERROR_UNEXPECTED);

    mDestListener = fromListener;
  }

  if (xhr) {
    static_cast<nsXMLHttpRequest*>(xhr.get())->mState |=
      XML_HTTP_REQUEST_MPART_HEADERS;
   }

  return mDestListener->OnStartRequest(aRequest, ctxt);
}

NS_IMETHODIMP
nsMultipartProxyListener::OnStopRequest(nsIRequest *aRequest,
                                        nsISupports *ctxt,
                                        nsresult status)
{
  return mDestListener->OnStopRequest(aRequest, ctxt, status);
}



NS_IMETHODIMP
nsMultipartProxyListener::OnDataAvailable(nsIRequest *aRequest,
                                          nsISupports *ctxt,
                                          nsIInputStream *inStr,
                                          PRUint32 sourceOffset,
                                          PRUint32 count)
{
  return mDestListener->OnDataAvailable(aRequest, ctxt, inStr, sourceOffset,
                                        count);
}



NS_IMPL_CYCLE_COLLECTION_CLASS(nsXHREventTarget)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXHREventTarget,
                                                  nsDOMEventTargetWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLoadListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnAbortListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLoadStartListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnProgressListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLoadendListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXHREventTarget,
                                                nsDOMEventTargetWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLoadListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnAbortListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLoadStartListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnProgressListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLoadendListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXHREventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequestEventTarget)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetWrapperCache)

NS_IMPL_ADDREF_INHERITED(nsXHREventTarget, nsDOMEventTargetWrapperCache)
NS_IMPL_RELEASE_INHERITED(nsXHREventTarget, nsDOMEventTargetWrapperCache)

NS_IMETHODIMP
nsXHREventTarget::GetOnload(nsIDOMEventListener** aOnLoad)
{
  return GetInnerEventListener(mOnLoadListener, aOnLoad);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnload(nsIDOMEventListener* aOnLoad)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(LOAD_STR),
                                mOnLoadListener, aOnLoad);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnerror(nsIDOMEventListener** aOnerror)
{
  return GetInnerEventListener(mOnErrorListener, aOnerror);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnerror(nsIDOMEventListener* aOnerror)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(ERROR_STR),
                                mOnErrorListener, aOnerror);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnabort(nsIDOMEventListener** aOnabort)
{
  return GetInnerEventListener(mOnAbortListener, aOnabort);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnabort(nsIDOMEventListener* aOnabort)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(ABORT_STR),
                                mOnAbortListener, aOnabort);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnloadstart(nsIDOMEventListener** aOnloadstart)
{
  return GetInnerEventListener(mOnLoadStartListener, aOnloadstart);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnloadstart(nsIDOMEventListener* aOnloadstart)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(LOADSTART_STR),
                                mOnLoadStartListener, aOnloadstart);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnprogress(nsIDOMEventListener** aOnprogress)
{
  return GetInnerEventListener(mOnProgressListener, aOnprogress);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnprogress(nsIDOMEventListener* aOnprogress)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(PROGRESS_STR),
                                mOnProgressListener, aOnprogress);
}

NS_IMETHODIMP
nsXHREventTarget::GetOnloadend(nsIDOMEventListener** aOnLoadend)
{
  return GetInnerEventListener(mOnLoadendListener, aOnLoadend);
}

NS_IMETHODIMP
nsXHREventTarget::SetOnloadend(nsIDOMEventListener* aOnLoadend)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(LOADEND_STR),
                                mOnLoadendListener, aOnLoadend);
}



nsXMLHttpRequestUpload::~nsXMLHttpRequestUpload()
{
  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}

DOMCI_DATA(XMLHttpRequestUpload, nsXMLHttpRequestUpload)

NS_INTERFACE_MAP_BEGIN(nsXMLHttpRequestUpload)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequestUpload)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XMLHttpRequestUpload)
NS_INTERFACE_MAP_END_INHERITING(nsXHREventTarget)

NS_IMPL_ADDREF_INHERITED(nsXMLHttpRequestUpload, nsXHREventTarget)
NS_IMPL_RELEASE_INHERITED(nsXMLHttpRequestUpload, nsXHREventTarget)






nsXMLHttpRequest::nsXMLHttpRequest()
  : mResponseType(XML_HTTP_RESPONSE_TYPE_DEFAULT),
    mRequestObserver(nsnull), mState(XML_HTTP_REQUEST_UNSENT),
    mUploadTransferred(0), mUploadTotal(0), mUploadComplete(PR_TRUE),
    mUploadProgress(0), mUploadProgressMax(0),
    mErrorLoad(PR_FALSE), mTimerIsActive(PR_FALSE),
    mProgressEventWasDelayed(PR_FALSE),
    mLoadLengthComputable(PR_FALSE), mLoadTotal(0),
    mFirstStartRequestSeen(PR_FALSE)
{
  mResponseBodyUnicode.SetIsVoid(PR_TRUE);
  nsLayoutStatics::AddRef();
}

nsXMLHttpRequest::~nsXMLHttpRequest()
{
  if (mListenerManager) {
    mListenerManager->Disconnect();
  }

  if (mState & (XML_HTTP_REQUEST_STOPPED |
                XML_HTTP_REQUEST_SENT |
                XML_HTTP_REQUEST_LOADING)) {
    Abort();
  }

  NS_ABORT_IF_FALSE(!(mState & XML_HTTP_REQUEST_SYNCLOOPING), "we rather crash than hang");
  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  nsLayoutStatics::Release();
}




nsresult
nsXMLHttpRequest::Init()
{
  
  
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");

  if (!stack) {
    return NS_OK;
  }

  JSContext *cx;

  if (NS_FAILED(stack->Peek(&cx)) || !cx) {
    return NS_OK;
  }

  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  if (secMan) {
    secMan->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  }
  NS_ENSURE_STATE(subjectPrincipal);
  mPrincipal = subjectPrincipal;

  nsIScriptContext* context = GetScriptContextFromJSContext(cx);
  if (context) {
    mScriptContext = context;
    nsCOMPtr<nsPIDOMWindow> window =
      do_QueryInterface(context->GetGlobalObject());
    if (window) {
      mOwner = window->GetCurrentInnerWindow();
    }
  }

  return NS_OK;
}



NS_IMETHODIMP
nsXMLHttpRequest::Init(nsIPrincipal* aPrincipal,
                       nsIScriptContext* aScriptContext,
                       nsPIDOMWindow* aOwnerWindow,
                       nsIURI* aBaseURI)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);

  
  
  

  mPrincipal = aPrincipal;
  mScriptContext = aScriptContext;
  if (aOwnerWindow) {
    mOwner = aOwnerWindow->GetCurrentInnerWindow();
  }
  else {
    mOwner = nsnull;
  }
  mBaseURI = aBaseURI;

  return NS_OK;
}




NS_IMETHODIMP
nsXMLHttpRequest::Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                             PRUint32 argc, jsval *argv)
{
  mOwner = do_QueryInterface(aOwner);
  if (!mOwner) {
    NS_WARNING("Unexpected nsIJSNativeInitializer owner");
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIScriptObjectPrincipal> scriptPrincipal = do_QueryInterface(aOwner);
  NS_ENSURE_STATE(scriptPrincipal);
  mPrincipal = scriptPrincipal->GetPrincipal();
  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aOwner);
  NS_ENSURE_STATE(sgo);
  mScriptContext = sgo->GetContext();
  NS_ENSURE_STATE(mScriptContext);
  return NS_OK; 
}

void
nsXMLHttpRequest::SetRequestObserver(nsIRequestObserver* aObserver)
{
  mRequestObserver = aObserver;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLHttpRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXMLHttpRequest,
                                                  nsXHREventTarget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannel)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mReadRequest)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mResponseXML)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCORSPreflightChannel)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnUploadProgressListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnReadystatechangeListener)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mXMLParserStreamListener)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mProgressEventSink)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mUpload,
                                                       nsIXMLHttpRequestUpload)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXMLHttpRequest,
                                                nsXHREventTarget)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChannel)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mReadRequest)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mResponseXML)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCORSPreflightChannel)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnUploadProgressListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnReadystatechangeListener)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mXMLParserStreamListener)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mProgressEventSink)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mUpload)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

DOMCI_DATA(XMLHttpRequest, nsXMLHttpRequest)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIJSXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XMLHttpRequest)
NS_INTERFACE_MAP_END_INHERITING(nsXHREventTarget)

NS_IMPL_ADDREF_INHERITED(nsXMLHttpRequest, nsXHREventTarget)
NS_IMPL_RELEASE_INHERITED(nsXMLHttpRequest, nsXHREventTarget)

NS_IMETHODIMP
nsXMLHttpRequest::GetOnreadystatechange(nsIDOMEventListener * *aOnreadystatechange)
{
  return
    nsXHREventTarget::GetInnerEventListener(mOnReadystatechangeListener,
                                            aOnreadystatechange);
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnreadystatechange(nsIDOMEventListener * aOnreadystatechange)
{
  return
    nsXHREventTarget::RemoveAddEventListener(NS_LITERAL_STRING(READYSTATE_STR),
                                             mOnReadystatechangeListener,
                                             aOnreadystatechange);
}

NS_IMETHODIMP
nsXMLHttpRequest::GetOnuploadprogress(nsIDOMEventListener * *aOnuploadprogress)
{
  return
    nsXHREventTarget::GetInnerEventListener(mOnUploadProgressListener,
                                            aOnuploadprogress);
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnuploadprogress(nsIDOMEventListener * aOnuploadprogress)
{
  return
    nsXHREventTarget::RemoveAddEventListener(NS_LITERAL_STRING(UPLOADPROGRESS_STR),
                                             mOnUploadProgressListener,
                                             aOnuploadprogress);
}


NS_IMETHODIMP
nsXMLHttpRequest::GetChannel(nsIChannel **aChannel)
{
  NS_ENSURE_ARG_POINTER(aChannel);
  NS_IF_ADDREF(*aChannel = mChannel);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetResponseXML(nsIDOMDocument **aResponseXML)
{
  NS_ENSURE_ARG_POINTER(aResponseXML);
  *aResponseXML = nsnull;
  if (mResponseType != XML_HTTP_RESPONSE_TYPE_DEFAULT &&
      mResponseType != XML_HTTP_RESPONSE_TYPE_DOCUMENT) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }
  if ((XML_HTTP_REQUEST_DONE & mState) && mResponseXML) {
    *aResponseXML = mResponseXML;
    NS_ADDREF(*aResponseXML);
  }

  return NS_OK;
}





nsresult
nsXMLHttpRequest::DetectCharset(nsACString& aCharset)
{
  aCharset.Truncate();
  nsresult rv;
  nsCAutoString charsetVal;
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(mReadRequest));
  if (!channel) {
    channel = mChannel;
    if (!channel) {
      
      
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  rv = channel->GetContentCharset(charsetVal);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsICharsetAlias> calias(do_GetService(NS_CHARSETALIAS_CONTRACTID,&rv));
    if(NS_SUCCEEDED(rv) && calias) {
      rv = calias->GetPreferred(charsetVal, aCharset);
    }
  }
  return rv;
}

nsresult
nsXMLHttpRequest::ConvertBodyToText(nsAString& aOutBuffer)
{
  
  
  
  
  if (!mResponseBodyUnicode.IsVoid()) {
    aOutBuffer = mResponseBodyUnicode;
    return NS_OK;
  }
  
  PRInt32 dataLen = mResponseBody.Length();
  if (!dataLen) {
    mResponseBodyUnicode.SetIsVoid(PR_FALSE);
    return NS_OK;
  }

  nsresult rv = NS_OK;

  nsCAutoString dataCharset;
  nsCOMPtr<nsIDocument> document(do_QueryInterface(mResponseXML));
  if (document) {
    dataCharset = document->GetDocumentCharacterSet();
  } else {
    if (NS_FAILED(DetectCharset(dataCharset)) || dataCharset.IsEmpty()) {
      
      dataCharset.AssignLiteral("UTF-8");
    }
  }

  
  if (dataCharset.EqualsLiteral("ASCII")) {
    CopyASCIItoUTF16(mResponseBody, mResponseBodyUnicode);
    aOutBuffer = mResponseBodyUnicode;
    return NS_OK;
  }

  
  
  

  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIUnicodeDecoder> decoder;
  rv = ccm->GetUnicodeDecoderRaw(dataCharset.get(),
                                 getter_AddRefs(decoder));
  if (NS_FAILED(rv))
    return rv;

  const char * inBuffer = mResponseBody.get();
  PRInt32 outBufferLength;
  rv = decoder->GetMaxLength(inBuffer, dataLen, &outBufferLength);
  if (NS_FAILED(rv))
    return rv;

  nsStringBuffer* buf =
    nsStringBuffer::Alloc((outBufferLength + 1) * sizeof(PRUnichar));
  if (!buf) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRUnichar* outBuffer = static_cast<PRUnichar*>(buf->Data());

  PRInt32 totalChars = 0,
          outBufferIndex = 0,
          outLen = outBufferLength;

  do {
    PRInt32 inBufferLength = dataLen;
    rv = decoder->Convert(inBuffer,
                          &inBufferLength,
                          &outBuffer[outBufferIndex],
                          &outLen);
    totalChars += outLen;
    if (NS_FAILED(rv)) {
      
      
      outBuffer[outBufferIndex + outLen++] = (PRUnichar)0xFFFD;
      outBufferIndex += outLen;
      outLen = outBufferLength - (++totalChars);

      decoder->Reset();

      if((inBufferLength + 1) > dataLen) {
        inBufferLength = dataLen;
      } else {
        inBufferLength++;
      }

      inBuffer = &inBuffer[inBufferLength];
      dataLen -= inBufferLength;
    }
  } while ( NS_FAILED(rv) && (dataLen > 0) );

  
  
  if (outBufferLength < 127 ||
      (outBufferLength * 0.9) < totalChars) {
    outBuffer[totalChars] = PRUnichar(0);
    
    buf->ToString(totalChars, mResponseBodyUnicode, PR_TRUE);
  } else {
    mResponseBodyUnicode.Assign(outBuffer, totalChars);
    buf->Release();
  }
  aOutBuffer = mResponseBodyUnicode;
  return NS_OK;
}


NS_IMETHODIMP nsXMLHttpRequest::GetResponseText(nsAString& aResponseText)
{
  nsresult rv = NS_OK;

  aResponseText.Truncate();

  if (mResponseType != XML_HTTP_RESPONSE_TYPE_DEFAULT &&
      mResponseType != XML_HTTP_RESPONSE_TYPE_TEXT) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (mState & (XML_HTTP_REQUEST_DONE |
                XML_HTTP_REQUEST_LOADING)) {
    rv = ConvertBodyToText(aResponseText);
  }

  return rv;
}

nsresult nsXMLHttpRequest::GetResponseArrayBuffer(jsval *aResult)
{
  JSContext *cx = nsContentUtils::GetCurrentJSContext();
  if (!cx)
    return NS_ERROR_FAILURE;

  if (!(mState & (XML_HTTP_REQUEST_DONE |
                  XML_HTTP_REQUEST_LOADING))) {
    *aResult = JSVAL_NULL;
    return NS_OK;
  }

  PRInt32 dataLen = mResponseBody.Length();
  JSObject *obj = js_CreateArrayBuffer(cx, dataLen);
  if (!obj)
    return NS_ERROR_FAILURE;

  *aResult = OBJECT_TO_JSVAL(obj);

  if (dataLen > 0) {
    js::ArrayBuffer *abuf = js::ArrayBuffer::fromJSObject(obj);
    NS_ASSERTION(abuf, "What happened?");
    memcpy(abuf->data, mResponseBody.BeginReading(), dataLen);
  }

  return NS_OK;
}


NS_IMETHODIMP nsXMLHttpRequest::GetResponseType(nsAString& aResponseType)
{
  switch (mResponseType) {
  case XML_HTTP_RESPONSE_TYPE_DEFAULT:
    aResponseType.Truncate();
    break;
  case XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER:
    aResponseType.AssignLiteral("arraybuffer");
    break;
  case XML_HTTP_RESPONSE_TYPE_BLOB:
    aResponseType.AssignLiteral("blob");
    break;
  case XML_HTTP_RESPONSE_TYPE_DOCUMENT:
    aResponseType.AssignLiteral("document");
    break;
  case XML_HTTP_RESPONSE_TYPE_TEXT:
    aResponseType.AssignLiteral("text");
    break;
  default:
    NS_ERROR("Should not happen");
  }

  return NS_OK;
}


NS_IMETHODIMP nsXMLHttpRequest::SetResponseType(const nsAString& aResponseType)
{
  
  
  if (!(mState & (XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT |
                  XML_HTTP_REQUEST_HEADERS_RECEIVED)))
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  
  if (aResponseType.IsEmpty()) {
    mResponseType = XML_HTTP_RESPONSE_TYPE_DEFAULT;
  } else if (aResponseType.EqualsLiteral("arraybuffer")) {
    mResponseType = XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER;
  } else if (aResponseType.EqualsLiteral("blob")) {
    mResponseType = XML_HTTP_RESPONSE_TYPE_BLOB;
  } else if (aResponseType.EqualsLiteral("document")) {
    mResponseType = XML_HTTP_RESPONSE_TYPE_DOCUMENT;
  } else if (aResponseType.EqualsLiteral("text")) {
    mResponseType = XML_HTTP_RESPONSE_TYPE_TEXT;
  }
  
  

  
  
  
  
  
  if (mState & XML_HTTP_REQUEST_HEADERS_RECEIVED) {
    nsCOMPtr<nsICachingChannel> cc(do_QueryInterface(mChannel));
    if (cc) {
      cc->SetCacheAsFile(mResponseType == XML_HTTP_RESPONSE_TYPE_BLOB);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsXMLHttpRequest::GetResponse(JSContext *aCx, jsval *aResult)
{
  nsresult rv = NS_OK;

  switch (mResponseType) {
  case XML_HTTP_RESPONSE_TYPE_DEFAULT:
  case XML_HTTP_RESPONSE_TYPE_TEXT:
    {
      nsString str;
      rv = GetResponseText(str);
      if (NS_FAILED(rv)) return rv;
      nsStringBuffer* buf;
      *aResult = XPCStringConvert::ReadableToJSVal(aCx, str, &buf);
      if (buf) {
        str.ForgetSharedBuffer();
      }
    }
    break;

  case XML_HTTP_RESPONSE_TYPE_ARRAYBUFFER:
    if (mState & XML_HTTP_REQUEST_DONE) {
      rv = GetResponseArrayBuffer(aResult);
    } else {
      *aResult = JSVAL_NULL;
    }
    break;

  case XML_HTTP_RESPONSE_TYPE_BLOB:
    if (mState & XML_HTTP_REQUEST_DONE && mResponseBlob) {
      JSObject* scope = JS_GetScopeChain(aCx);
      rv = nsContentUtils::WrapNative(aCx, scope, mResponseBlob, aResult,
                                      nsnull, PR_TRUE);
    } else {
      *aResult = JSVAL_NULL;
    }
    break;

  case XML_HTTP_RESPONSE_TYPE_DOCUMENT:
    if (mState & XML_HTTP_REQUEST_DONE && mResponseXML) {
      JSObject* scope = JS_GetScopeChain(aCx);
      rv = nsContentUtils::WrapNative(aCx, scope, mResponseXML, aResult,
                                      nsnull, PR_TRUE);
    } else {
      *aResult = JSVAL_NULL;
    }
    break;

  default:
    NS_ERROR("Should not happen");
  }

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetStatus(PRUint32 *aStatus)
{
  *aStatus = 0;

  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    
    
    if (mChannel) {
      nsresult status;
      mChannel->GetStatus(&status);
      if (NS_FAILED(status)) {
        return NS_OK;
      }
    }
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (httpChannel) {
    nsresult rv = httpChannel->GetResponseStatus(aStatus);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      
      
      
      PRUint16 readyState;
      GetReadyState(&readyState);
      if (readyState >= LOADING) {
        *aStatus = 0;
        return NS_OK;
      }
    }

    return rv;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetStatusText(nsACString& aStatusText)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  aStatusText.Truncate();

  if (httpChannel) {
    if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
      
      
      if (mChannel) {
        nsresult status;
        mChannel->GetStatus(&status);
        if (NS_FAILED(status)) {
          return NS_OK;
        }
      }
    }

    httpChannel->GetResponseStatusText(aStatusText);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::Abort()
{
  if (mReadRequest) {
    mReadRequest->Cancel(NS_BINDING_ABORTED);
  }
  if (mChannel) {
    mChannel->Cancel(NS_BINDING_ABORTED);
  }
  if (mCORSPreflightChannel) {
    mCORSPreflightChannel->Cancel(NS_BINDING_ABORTED);
  }
  mResponseXML = nsnull;
  PRUint32 responseLength = mResponseBody.Length();
  mResponseBody.Truncate();
  mResponseBodyUnicode.SetIsVoid(PR_TRUE);
  mResponseBlob = nsnull;
  mState |= XML_HTTP_REQUEST_ABORTED;

  if (!(mState & (XML_HTTP_REQUEST_UNSENT |
                  XML_HTTP_REQUEST_OPENED |
                  XML_HTTP_REQUEST_DONE))) {
    ChangeState(XML_HTTP_REQUEST_DONE, PR_TRUE);
  }

  if (!(mState & XML_HTTP_REQUEST_SYNCLOOPING)) {
    NS_NAMED_LITERAL_STRING(abortStr, ABORT_STR);
    DispatchProgressEvent(this, abortStr, mLoadLengthComputable, responseLength,
                          mLoadTotal);
    if (mUpload && !mUploadComplete) {
      mUploadComplete = PR_TRUE;
      DispatchProgressEvent(mUpload, abortStr, PR_TRUE, mUploadTransferred,
                            mUploadTotal);
    }
  }

  
  
  
  if (mState & XML_HTTP_REQUEST_ABORTED) {
    ChangeState(XML_HTTP_REQUEST_UNSENT, PR_FALSE);  
  }

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetAllResponseHeaders(char **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    *_retval = ToNewCString(EmptyString());
    return NS_OK;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (httpChannel) {
    nsRefPtr<nsHeaderVisitor> visitor = new nsHeaderVisitor();
    nsresult rv = httpChannel->VisitResponseHeaders(visitor);
    if (NS_SUCCEEDED(rv))
      *_retval = ToNewCString(visitor->Headers());
  }
 
  if (!*_retval) {
    *_retval = ToNewCString(EmptyString());
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetResponseHeader(const nsACString& header,
                                    nsACString& _retval)
{
  nsresult rv = NS_OK;
  _retval.SetIsVoid(PR_TRUE);

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (!httpChannel) {
    return NS_OK;
  }

  
  PRBool chrome = PR_FALSE; 
  IsCapabilityEnabled("UniversalXPConnect", &chrome);
  if (!chrome &&
       (header.LowerCaseEqualsASCII("set-cookie") ||
        header.LowerCaseEqualsASCII("set-cookie2"))) {
    NS_WARNING("blocked access to response header");
    return NS_OK;
  }

  
  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    
    
    if (mChannel) {
      nsresult status;
      mChannel->GetStatus(&status);
      if (NS_FAILED(status)) {
        return NS_OK;
      }
    }

    const char *kCrossOriginSafeHeaders[] = {
      "cache-control", "content-language", "content-type", "expires",
      "last-modified", "pragma"
    };
    PRBool safeHeader = PR_FALSE;
    PRUint32 i;
    for (i = 0; i < NS_ARRAY_LENGTH(kCrossOriginSafeHeaders); ++i) {
      if (header.LowerCaseEqualsASCII(kCrossOriginSafeHeaders[i])) {
        safeHeader = PR_TRUE;
        break;
      }
    }

    if (!safeHeader) {
      nsCAutoString headerVal;
      
      
      httpChannel->
        GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Expose-Headers"),
                          headerVal);
      nsCCharSeparatedTokenizer exposeTokens(headerVal, ',');
      while(exposeTokens.hasMoreTokens()) {
        const nsDependentCSubstring& token = exposeTokens.nextToken();
        if (token.IsEmpty()) {
          continue;
        }
        if (!IsValidHTTPToken(token)) {
          return NS_OK;
        }
        if (header.Equals(token, nsCaseInsensitiveCStringComparator())) {
          safeHeader = PR_TRUE;
        }
      }
    }

    if (!safeHeader) {
      return NS_OK;
    }
  }

  rv = httpChannel->GetResponseHeader(header, _retval);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    
    _retval.SetIsVoid(PR_TRUE);
    rv = NS_OK;
  }

  return rv;
}

nsresult
nsXMLHttpRequest::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
  NS_ENSURE_ARG_POINTER(aLoadGroup);
  *aLoadGroup = nsnull;

  if (mState & XML_HTTP_REQUEST_BACKGROUND) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(mScriptContext);
  if (doc) {
    *aLoadGroup = doc->GetDocumentLoadGroup().get();  
  }

  return NS_OK;
}

nsresult
nsXMLHttpRequest::CreateReadystatechangeEvent(nsIDOMEvent** aDOMEvent)
{
  nsresult rv = nsEventDispatcher::CreateEvent(nsnull, nsnull,
                                               NS_LITERAL_STRING("Events"),
                                               aDOMEvent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIPrivateDOMEvent> privevent(do_QueryInterface(*aDOMEvent));
  if (!privevent) {
    NS_IF_RELEASE(*aDOMEvent);
    return NS_ERROR_FAILURE;
  }

  (*aDOMEvent)->InitEvent(NS_LITERAL_STRING(READYSTATE_STR),
                          PR_FALSE, PR_FALSE);

  
  privevent->SetTrusted(PR_TRUE);

  return NS_OK;
}

void
nsXMLHttpRequest::DispatchProgressEvent(nsPIDOMEventTarget* aTarget,
                                        const nsAString& aType,
                                        PRBool aUseLSEventWrapper,
                                        PRBool aLengthComputable,
                                        PRUint64 aLoaded, PRUint64 aTotal,
                                        PRUint64 aPosition, PRUint64 aTotalSize)
{
  NS_ASSERTION(aTarget, "null target");
  if (aType.IsEmpty() ||
      (!AllowUploadProgress() &&
       (aTarget == mUpload || aType.EqualsLiteral(UPLOADPROGRESS_STR)))) {
    return;
  }

  PRBool dispatchLoadend = aType.EqualsLiteral(LOAD_STR) ||
                           aType.EqualsLiteral(ERROR_STR) ||
                           aType.EqualsLiteral(ABORT_STR);
  
  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = nsEventDispatcher::CreateEvent(nsnull, nsnull,
                                               NS_LITERAL_STRING("ProgressEvent"),
                                               getter_AddRefs(event));
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIPrivateDOMEvent> privevent(do_QueryInterface(event));
  if (!privevent) {
    return;
  }
  privevent->SetTrusted(PR_TRUE);

  nsCOMPtr<nsIDOMProgressEvent> progress = do_QueryInterface(event);
  if (!progress) {
    return;
  }

  progress->InitProgressEvent(aType, PR_FALSE, PR_FALSE, aLengthComputable,
                              aLoaded, (aTotal == LL_MAXUINT) ? 0 : aTotal);

  if (aUseLSEventWrapper) {
    nsCOMPtr<nsIDOMProgressEvent> xhrprogressEvent =
      new nsXMLHttpProgressEvent(progress, aPosition, aTotalSize);
    if (!xhrprogressEvent) {
      return;
    }
    event = xhrprogressEvent;
  }
  aTarget->DispatchDOMEvent(nsnull, event, nsnull, nsnull);
  
  if (dispatchLoadend) {
    DispatchProgressEvent(aTarget, NS_LITERAL_STRING(LOADEND_STR),
                          aUseLSEventWrapper, aLengthComputable,
                          aLoaded, aTotal, aPosition, aTotalSize);
  }
}
                                          
already_AddRefed<nsIHttpChannel>
nsXMLHttpRequest::GetCurrentHttpChannel()
{
  nsIHttpChannel *httpChannel = nsnull;

  if (mReadRequest) {
    CallQueryInterface(mReadRequest, &httpChannel);
  }

  if (!httpChannel && mChannel) {
    CallQueryInterface(mChannel, &httpChannel);
  }

  return httpChannel;
}

nsresult
nsXMLHttpRequest::CheckChannelForCrossSiteRequest(nsIChannel* aChannel)
{
  
  if (IsSystemXHR()) {
    return NS_OK;
  }

  
  if (nsContentUtils::CheckMayLoad(mPrincipal, aChannel)) {
    return NS_OK;
  }

  
  mState |= XML_HTTP_REQUEST_USE_XSITE_AC;

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  NS_ENSURE_TRUE(httpChannel, NS_ERROR_DOM_BAD_URI);
    
  nsCAutoString method;
  httpChannel->GetRequestMethod(method);
  if (!mCORSUnsafeHeaders.IsEmpty() ||
      HasListenersFor(NS_LITERAL_STRING(UPLOADPROGRESS_STR)) ||
      (mUpload && mUpload->HasListeners()) ||
      (!method.LowerCaseEqualsLiteral("get") &&
       !method.LowerCaseEqualsLiteral("post") &&
       !method.LowerCaseEqualsLiteral("head"))) {
    mState |= XML_HTTP_REQUEST_NEED_AC_PREFLIGHT;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::Open(const nsACString& method, const nsACString& url,
                       PRBool async, const nsAString& user,
                       const nsAString& password, PRUint8 optional_argc)
{
  NS_ENSURE_ARG(!method.IsEmpty());

  if (!optional_argc) {
    
    async = PR_TRUE;
  }

  NS_ENSURE_TRUE(mPrincipal, NS_ERROR_NOT_INITIALIZED);

  
  
  if (method.LowerCaseEqualsLiteral("trace") ||
      method.LowerCaseEqualsLiteral("track")) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  PRBool authp = PR_FALSE;

  if (mState & (XML_HTTP_REQUEST_OPENED |
                XML_HTTP_REQUEST_HEADERS_RECEIVED |
                XML_HTTP_REQUEST_LOADING |
                XML_HTTP_REQUEST_SENT |
                XML_HTTP_REQUEST_STOPPED)) {
    
    Abort();

    
    
    
    
    
  }

  if (mState & XML_HTTP_REQUEST_ABORTED) {
    
    
    
    

    mState &= ~XML_HTTP_REQUEST_ABORTED;
  }

  if (async) {
    mState |= XML_HTTP_REQUEST_ASYNC;
  } else {
    mState &= ~XML_HTTP_REQUEST_ASYNC;
  }

  mState &= ~XML_HTTP_REQUEST_MPART_HEADERS;

  nsCOMPtr<nsIDocument> doc =
    nsContentUtils::GetDocumentFromScriptContext(mScriptContext);
  
  nsCOMPtr<nsIURI> baseURI;
  if (mBaseURI) {
    baseURI = mBaseURI;
  }
  else if (doc) {
    baseURI = doc->GetBaseURI();
  }

  rv = NS_NewURI(getter_AddRefs(uri), url, nsnull, baseURI);
  if (NS_FAILED(rv)) return rv;

  
  
  rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_XMLHTTPREQUEST,
                                 uri,
                                 mPrincipal,
                                 doc,
                                 EmptyCString(), 
                                 nsnull,         
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv)) return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  if (!user.IsEmpty()) {
    nsCAutoString userpass;
    CopyUTF16toUTF8(user, userpass);
    if (!password.IsEmpty()) {
      userpass.Append(':');
      AppendUTF16toUTF8(password, userpass);
    }
    uri->SetUserPass(userpass);
    authp = PR_TRUE;
  }

  
  
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  GetLoadGroup(getter_AddRefs(loadGroup));

  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = mPrincipal->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_XMLHTTPREQUEST);
  }
  rv = NS_NewChannel(getter_AddRefs(mChannel),
                     uri,
                     nsnull,                    
                     loadGroup,
                     nsnull,                    
                     nsIRequest::LOAD_BACKGROUND,
                     channelPolicy);
  if (NS_FAILED(rv)) return rv;

  mState &= ~(XML_HTTP_REQUEST_USE_XSITE_AC |
              XML_HTTP_REQUEST_NEED_AC_PREFLIGHT);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (httpChannel) {
    rv = httpChannel->SetRequestMethod(method);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  ChangeState(XML_HTTP_REQUEST_OPENED);

  return rv;
}




NS_METHOD
nsXMLHttpRequest::StreamReaderFunc(nsIInputStream* in,
                                   void* closure,
                                   const char* fromRawSegment,
                                   PRUint32 toOffset,
                                   PRUint32 count,
                                   PRUint32 *writeCount)
{
  nsXMLHttpRequest* xmlHttpRequest = static_cast<nsXMLHttpRequest*>(closure);
  if (!xmlHttpRequest || !writeCount) {
    NS_WARNING("XMLHttpRequest cannot read from stream: no closure or writeCount");
    return NS_ERROR_FAILURE;
  }

  if (xmlHttpRequest->mResponseType == XML_HTTP_RESPONSE_TYPE_BLOB &&
      xmlHttpRequest->mResponseBlob) {
    xmlHttpRequest->ChangeState(XML_HTTP_REQUEST_LOADING);
    *writeCount = count;
    return NS_OK;
  }

  if (xmlHttpRequest->mResponseType != XML_HTTP_RESPONSE_TYPE_DOCUMENT) {
    
    PRUint32 previousLength = xmlHttpRequest->mResponseBody.Length();
    xmlHttpRequest->mResponseBody.Append(fromRawSegment,count);
    if (count > 0 && xmlHttpRequest->mResponseBody.Length() == previousLength) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    xmlHttpRequest->mResponseBodyUnicode.SetIsVoid(PR_TRUE);
  }

  nsresult rv = NS_OK;

  if (xmlHttpRequest->mState & XML_HTTP_REQUEST_PARSEBODY) {
    

    
    
    
    nsCOMPtr<nsIInputStream> copyStream;
    rv = NS_NewByteInputStream(getter_AddRefs(copyStream), fromRawSegment, count);

    if (NS_SUCCEEDED(rv) && xmlHttpRequest->mXMLParserStreamListener) {
      NS_ASSERTION(copyStream, "NS_NewByteInputStream lied");
      nsresult parsingResult = xmlHttpRequest->mXMLParserStreamListener
                                  ->OnDataAvailable(xmlHttpRequest->mReadRequest,
                                                    xmlHttpRequest->mContext,
                                                    copyStream, toOffset, count);

      
      
      if (NS_FAILED(parsingResult)) {
        xmlHttpRequest->mState &= ~XML_HTTP_REQUEST_PARSEBODY;
      }
    }
  }

  xmlHttpRequest->ChangeState(XML_HTTP_REQUEST_LOADING);

  if (NS_SUCCEEDED(rv)) {
    *writeCount = count;
  } else {
    *writeCount = 0;
  }

  return rv;
}

void nsXMLHttpRequest::CreateResponseBlob(nsIRequest *request)
{
  nsCOMPtr<nsIFile> file;
  nsCOMPtr<nsICachingChannel> cc(do_QueryInterface(request));
  if (cc) {
    cc->GetCacheFile(getter_AddRefs(file));
    if (!file) {
      
      PRBool cacheAsFile = PR_FALSE;
      if (NS_SUCCEEDED(cc->GetCacheAsFile(&cacheAsFile)) && cacheAsFile) {
        
      }
    }
  } else {
    nsCOMPtr<nsIFileChannel> fc = do_QueryInterface(request);
    if (fc) {
      fc->GetFile(getter_AddRefs(file));
    }
  }
  if (file) {
    nsCAutoString contentType;
    mChannel->GetContentType(contentType);
    mResponseBlob = new nsDOMFile(file,
                                  NS_ConvertASCIItoUTF16(contentType));
    mResponseBody.Truncate();
    mResponseBodyUnicode.SetIsVoid(PR_TRUE);
  }
}


NS_IMETHODIMP
nsXMLHttpRequest::OnDataAvailable(nsIRequest *request, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  NS_ENSURE_ARG_POINTER(inStr);

  NS_ABORT_IF_FALSE(mContext.get() == ctxt,"start context different from OnDataAvailable context");

  if (mResponseType == XML_HTTP_RESPONSE_TYPE_BLOB && !mResponseBlob) {
    CreateResponseBlob(request);
  }

  PRUint32 totalRead;
  return inStr->ReadSegments(nsXMLHttpRequest::StreamReaderFunc, (void*)this, count, &totalRead);
}

PRBool
IsSameOrBaseChannel(nsIRequest* aPossibleBase, nsIChannel* aChannel)
{
  nsCOMPtr<nsIMultiPartChannel> mpChannel = do_QueryInterface(aPossibleBase);
  if (mpChannel) {
    nsCOMPtr<nsIChannel> baseChannel;
    nsresult rv = mpChannel->GetBaseChannel(getter_AddRefs(baseChannel));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    
    return baseChannel == aChannel;
  }

  return aPossibleBase == aChannel;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  nsresult rv = NS_OK;
  if (!mFirstStartRequestSeen && mRequestObserver) {
    mFirstStartRequestSeen = PR_TRUE;
    mRequestObserver->OnStartRequest(request, ctxt);
  }

  if (!IsSameOrBaseChannel(request, mChannel)) {
    return NS_OK;
  }

  
  if (mState & XML_HTTP_REQUEST_UNSENT)
    return NS_OK;

  if (mState & XML_HTTP_REQUEST_ABORTED) {
    NS_ERROR("Ugh, still getting data on an aborted XMLHttpRequest!");

    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIPrincipal> documentPrincipal;
  if (IsSystemXHR()) {
    
    
    
    
    nsresult rv;
    documentPrincipal = do_CreateInstance("@mozilla.org/nullprincipal;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    documentPrincipal = mPrincipal;
  }

  channel->SetOwner(documentPrincipal);

  mReadRequest = request;
  mContext = ctxt;
  mState |= XML_HTTP_REQUEST_PARSEBODY;
  mState &= ~XML_HTTP_REQUEST_MPART_HEADERS;
  ChangeState(XML_HTTP_REQUEST_HEADERS_RECEIVED);

  if (mResponseType == XML_HTTP_RESPONSE_TYPE_BLOB) {
    nsCOMPtr<nsICachingChannel> cc(do_QueryInterface(mChannel));
    if (cc) {
      cc->SetCacheAsFile(PR_TRUE);
    }
  }

  nsresult status;
  request->GetStatus(&status);
  mErrorLoad = mErrorLoad || NS_FAILED(status);

  if (mUpload && !mUploadComplete && !mErrorLoad &&
      (mState & XML_HTTP_REQUEST_ASYNC)) {
    mUploadComplete = PR_TRUE;
    DispatchProgressEvent(mUpload, NS_LITERAL_STRING(LOAD_STR),
                          PR_TRUE, mUploadTotal, mUploadTotal);
  }

  
  mResponseBody.Truncate();
  mResponseBodyUnicode.SetIsVoid(PR_TRUE);
  mResponseBlob = nsnull;

  
  PRBool parseBody = mResponseType == XML_HTTP_RESPONSE_TYPE_DEFAULT ||
                     mResponseType == XML_HTTP_RESPONSE_TYPE_DOCUMENT;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (parseBody && httpChannel) {
    nsCAutoString method;
    httpChannel->GetRequestMethod(method);
    parseBody = !method.EqualsLiteral("HEAD");
  }

  if (parseBody && NS_SUCCEEDED(status)) {
    if (!mOverrideMimeType.IsEmpty()) {
      channel->SetContentType(mOverrideMimeType);
    }

    
    
    
    
    nsCAutoString type;
    channel->GetContentType(type);

    if (type.Find("xml") == kNotFound) {
      mState &= ~XML_HTTP_REQUEST_PARSEBODY;
    }
  } else {
    
    mState &= ~XML_HTTP_REQUEST_PARSEBODY;
  }

  if (mState & XML_HTTP_REQUEST_PARSEBODY) {
    nsCOMPtr<nsIURI> baseURI, docURI;
    nsCOMPtr<nsIDocument> doc =
      nsContentUtils::GetDocumentFromScriptContext(mScriptContext);

    if (doc) {
      docURI = doc->GetDocumentURI();
      baseURI = doc->GetBaseURI();
    }

    
    
    
    
    const nsAString& emptyStr = EmptyString();
    nsCOMPtr<nsIScriptGlobalObject> global = do_QueryInterface(mOwner);
    rv = nsContentUtils::CreateDocument(emptyStr, emptyStr, nsnull, docURI,
                                        baseURI, mPrincipal, global,
                                        getter_AddRefs(mResponseXML));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> responseDoc = do_QueryInterface(mResponseXML);
    responseDoc->SetPrincipal(documentPrincipal);

    if (IsSystemXHR()) {
      responseDoc->ForceEnableXULXBL();
    }

    if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
      nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mResponseXML);
      if (htmlDoc) {
        htmlDoc->DisableCookieAccess();
      }
    }

    
    nsCOMPtr<nsPIDOMEventTarget> target(do_QueryInterface(mResponseXML));
    if (target) {
      nsWeakPtr requestWeak =
        do_GetWeakReference(static_cast<nsIXMLHttpRequest*>(this));
      nsCOMPtr<nsIDOMEventListener> proxy = new nsLoadListenerProxy(requestWeak);
      if (!proxy) return NS_ERROR_OUT_OF_MEMORY;

      
      rv = target->AddEventListenerByIID(static_cast<nsIDOMEventListener*>
                                                    (proxy),
                                         NS_GET_IID(nsIDOMLoadListener));
      NS_ENSURE_SUCCESS(rv, rv);
    }



    nsCOMPtr<nsIStreamListener> listener;
    nsCOMPtr<nsILoadGroup> loadGroup;
    channel->GetLoadGroup(getter_AddRefs(loadGroup));

    rv = responseDoc->StartDocumentLoad(kLoadAsData, channel, loadGroup,
                                        nsnull, getter_AddRefs(listener),
                                        !(mState & XML_HTTP_REQUEST_USE_XSITE_AC));
    NS_ENSURE_SUCCESS(rv, rv);

    mXMLParserStreamListener = listener;
    rv = mXMLParserStreamListener->OnStartRequest(request, ctxt);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (NS_SUCCEEDED(rv) &&
      (mState & XML_HTTP_REQUEST_ASYNC) &&
      HasListenersFor(NS_LITERAL_STRING(PROGRESS_STR))) {
    StartProgressEventTimer();
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
  if (!IsSameOrBaseChannel(request, mChannel)) {
    return NS_OK;
  }

  nsresult rv = NS_OK;

  
  
  
  
  
  
  nsCOMPtr<nsIMultiPartChannel> mpChannel = do_QueryInterface(request);
  if (mpChannel) {
    PRBool last;
    rv = mpChannel->GetIsLastPart(&last);
    NS_ENSURE_SUCCESS(rv, rv);
    if (last) {
      mState |= XML_HTTP_REQUEST_GOT_FINAL_STOP;
    }
  }
  else {
    mState |= XML_HTTP_REQUEST_GOT_FINAL_STOP;
  }

  if (mRequestObserver && mState & XML_HTTP_REQUEST_GOT_FINAL_STOP) {
    NS_ASSERTION(mFirstStartRequestSeen, "Inconsistent state!");
    mFirstStartRequestSeen = PR_FALSE;
    mRequestObserver->OnStopRequest(request, ctxt, status);
  }

  
  
  if (mState & XML_HTTP_REQUEST_UNSENT) {
    if (mXMLParserStreamListener)
      (void) mXMLParserStreamListener->OnStopRequest(request, ctxt, status);
    return NS_OK;
  }

  nsCOMPtr<nsIParser> parser;

  
  if (mState & XML_HTTP_REQUEST_PARSEBODY && mXMLParserStreamListener) {
    parser = do_QueryInterface(mXMLParserStreamListener);
    NS_ABORT_IF_FALSE(parser, "stream listener was expected to be a parser");
    rv = mXMLParserStreamListener->OnStopRequest(request, ctxt, status);
  }

  mXMLParserStreamListener = nsnull;
  mReadRequest = nsnull;
  mContext = nsnull;

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  if (NS_SUCCEEDED(status) && mResponseType == XML_HTTP_RESPONSE_TYPE_BLOB) {
    if (!mResponseBlob) {
      CreateResponseBlob(request);
    }
    if (!mResponseBlob) {
      
      
      nsCAutoString contentType;
      mChannel->GetContentType(contentType);
      
      
      PRUint32 blobLen = mResponseBody.Length();
      void *blobData = PR_Malloc(blobLen);
      if (blobData) {
        memcpy(blobData, mResponseBody.BeginReading(), blobLen);
        mResponseBlob =
          new nsDOMMemoryFile(blobData, blobLen, EmptyString(),
                              NS_ConvertASCIItoUTF16(contentType));
        mResponseBody.Truncate();
      }
      NS_ASSERTION(mResponseBodyUnicode.IsVoid(),
                   "mResponseBodyUnicode should be empty");
    }
  }

  channel->SetNotificationCallbacks(nsnull);
  mNotificationCallbacks = nsnull;
  mChannelEventSink = nsnull;
  mProgressEventSink = nsnull;

  if (NS_FAILED(status)) {
    
    
    Error(nsnull);

    
    
    
    
    mChannel = nsnull;
  } else if (!parser || parser->IsParserEnabled()) {
    
    
    
    
    
    
    
    
    
    
    RequestCompleted();
  } else {
    ChangeState(XML_HTTP_REQUEST_STOPPED, PR_FALSE);
  }

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  return rv;
}

nsresult
nsXMLHttpRequest::RequestCompleted()
{
  nsresult rv = NS_OK;

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  
  
  
  if (mState & (XML_HTTP_REQUEST_UNSENT |
                XML_HTTP_REQUEST_DONE)) {
    return NS_OK;
  }

  
  
  
  
  if (mResponseXML) {
    nsCOMPtr<nsIDOMElement> root;
    mResponseXML->GetDocumentElement(getter_AddRefs(root));
    if (!root) {
      mResponseXML = nsnull;
    }
  }

  ChangeState(XML_HTTP_REQUEST_DONE, PR_TRUE);

  PRUint32 responseLength = mResponseBody.Length();
  NS_NAMED_LITERAL_STRING(errorStr, ERROR_STR);
  NS_NAMED_LITERAL_STRING(loadStr, LOAD_STR);
  DispatchProgressEvent(this,
                        mErrorLoad ? errorStr : loadStr,
                        !mErrorLoad,
                        responseLength,
                        mErrorLoad ? 0 : responseLength);
  if (mErrorLoad && mUpload && !mUploadComplete) {
    DispatchProgressEvent(mUpload, errorStr, PR_TRUE,
                          mUploadTransferred, mUploadTotal);
  }

  if (!(mState & XML_HTTP_REQUEST_GOT_FINAL_STOP)) {
    
    ChangeState(XML_HTTP_REQUEST_OPENED);
  }

  return rv;
}

NS_IMETHODIMP
nsXMLHttpRequest::SendAsBinary(const nsAString &aBody)
{
  char *data = static_cast<char*>(NS_Alloc(aBody.Length() + 1));
  if (!data)
    return NS_ERROR_OUT_OF_MEMORY;

  nsAString::const_iterator iter, end;
  aBody.BeginReading(iter);
  aBody.EndReading(end);
  char *p = data;
  while (iter != end) {
    if (*iter & 0xFF00) {
      NS_Free(data);
      return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
    }
    *p++ = static_cast<char>(*iter++);
  }
  *p = '\0';

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stream), data,
                                      aBody.Length(), NS_ASSIGNMENT_ADOPT);
  if (NS_FAILED(rv))
    NS_Free(data);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritableVariant> variant = new nsVariant();
  if (!variant) return NS_ERROR_OUT_OF_MEMORY;

  rv = variant->SetAsISupports(stream);
  NS_ENSURE_SUCCESS(rv, rv);

  return Send(variant);
}

static nsresult
GetRequestBody(nsIVariant* aBody, nsIInputStream** aResult,
               nsACString& aContentType, nsACString& aCharset)
{
  *aResult = nsnull;
  aContentType.AssignLiteral("text/plain");
  aCharset.AssignLiteral("UTF-8");

  PRUint16 dataType;
  nsresult rv = aBody->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dataType == nsIDataType::VTYPE_INTERFACE ||
      dataType == nsIDataType::VTYPE_INTERFACE_IS) {
    nsCOMPtr<nsISupports> supports;
    nsID *iid;
    rv = aBody->GetAsInterface(&iid, getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsMemory::Free(iid);

    
    nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(supports);
    if (doc) {
      aContentType.AssignLiteral("application/xml");
      nsAutoString inputEncoding;
      doc->GetInputEncoding(inputEncoding);
      if (!DOMStringIsNull(inputEncoding)) {
        CopyUTF16toUTF8(inputEncoding, aCharset);
      }

      
      
      nsCOMPtr<nsIDOMSerializer> serializer =
        do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIStorageStream> storStream;
      rv = NS_NewStorageStream(4096, PR_UINT32_MAX,
                               getter_AddRefs(storStream));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIOutputStream> output;
      rv = storStream->GetOutputStream(0, getter_AddRefs(output));
      NS_ENSURE_SUCCESS(rv, rv);

      
      rv = serializer->SerializeToStream(doc, output, aCharset);
      NS_ENSURE_SUCCESS(rv, rv);

      output->Close();

      return storStream->NewInputStream(0, aResult);
    }

    
    nsCOMPtr<nsISupportsString> wstr = do_QueryInterface(supports);
    if (wstr) {
      nsAutoString string;
      wstr->GetData(string);

      return NS_NewCStringInputStream(aResult,
                                      NS_ConvertUTF16toUTF8(string));
    }

    
    nsCOMPtr<nsIInputStream> stream = do_QueryInterface(supports);
    if (stream) {
      *aResult = stream.forget().get();
      aCharset.Truncate();

      return NS_OK;
    }

    
    nsCOMPtr<nsIXHRSendable> sendable = do_QueryInterface(supports);
    if (sendable) {
      return sendable->GetSendInfo(aResult, aContentType, aCharset);
    }
  }
  else if (dataType == nsIDataType::VTYPE_VOID ||
           dataType == nsIDataType::VTYPE_EMPTY) {
    
    return NS_OK;
  }

  PRUnichar* data = nsnull;
  PRUint32 len = 0;
  rv = aBody->GetAsWStringWithSize(&len, &data);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString string;
  string.Adopt(data, len);

  return NS_NewCStringInputStream(aResult, NS_ConvertUTF16toUTF8(string));
}


NS_IMETHODIMP
nsXMLHttpRequest::Send(nsIVariant *aBody)
{
  NS_ENSURE_TRUE(mPrincipal, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (XML_HTTP_REQUEST_SENT & mState) {
    return NS_ERROR_FAILURE;
  }

  
  if (!mChannel || !(XML_HTTP_REQUEST_OPENED & mState)) {
    return NS_ERROR_NOT_INITIALIZED;
  }


  
  
  
  
  if (HasListenersFor(NS_LITERAL_STRING(PROGRESS_STR)) ||
      HasListenersFor(NS_LITERAL_STRING(UPLOADPROGRESS_STR)) ||
      (mUpload && mUpload->HasListenersFor(NS_LITERAL_STRING(PROGRESS_STR)))) {
    nsLoadFlags loadFlags;
    mChannel->GetLoadFlags(&loadFlags);
    loadFlags &= ~nsIRequest::LOAD_BACKGROUND;
    loadFlags |= nsIRequest::LOAD_NORMAL;
    mChannel->SetLoadFlags(loadFlags);
  }

  
  
  

  
  
  nsCAutoString method;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));

  if (httpChannel) {
    httpChannel->GetRequestMethod(method); 

    if (!IsSystemXHR()) {
      
      
      
      
      
      
      
      
      
      
      

      nsCOMPtr<nsIURI> principalURI;
      mPrincipal->GetURI(getter_AddRefs(principalURI));

      nsCOMPtr<nsIDocument> doc =
        nsContentUtils::GetDocumentFromScriptContext(mScriptContext);

      nsCOMPtr<nsIURI> docCurURI;
      nsCOMPtr<nsIURI> docOrigURI;
      if (doc) {
        docCurURI = doc->GetDocumentURI();
        docOrigURI = doc->GetOriginalURI();
      }

      nsCOMPtr<nsIURI> referrerURI;

      if (principalURI && docCurURI && docOrigURI) {
        PRBool equal = PR_FALSE;
        principalURI->Equals(docOrigURI, &equal);
        if (equal) {
          referrerURI = docCurURI;
        }
      }

      if (!referrerURI)
        referrerURI = principalURI;

      httpChannel->SetReferrer(referrerURI);
    }

    
    
    
    
    
    
    
    nsCOMPtr<nsIUploadChannel2> uploadChannel2 =
      do_QueryInterface(httpChannel);
    if (!uploadChannel2) {
      nsCOMPtr<nsIConsoleService> consoleService =
        do_GetService(NS_CONSOLESERVICE_CONTRACTID);
      if (consoleService) {
        consoleService->LogStringMessage(NS_LITERAL_STRING(
          "Http channel implementation doesn't support nsIUploadChannel2. An extension has supplied a non-functional http protocol handler. This will break behavior and in future releases not work at all."
                                                           ).get());
      }
    }
  }

  mUploadTransferred = 0;
  mUploadTotal = 0;
  
  mUploadComplete = PR_TRUE;
  mErrorLoad = PR_FALSE;
  mLoadLengthComputable = PR_FALSE;
  mLoadTotal = 0;
  mUploadProgress = 0;
  mUploadProgressMax = 0;
  if (aBody && httpChannel && !method.EqualsLiteral("GET")) {

    nsCAutoString charset;
    nsCAutoString defaultContentType;
    nsCOMPtr<nsIInputStream> postDataStream;

    rv = GetRequestBody(aBody, getter_AddRefs(postDataStream),
                        defaultContentType, charset);
    NS_ENSURE_SUCCESS(rv, rv);

    if (postDataStream) {
      
      
      nsCAutoString contentType;
      if (NS_FAILED(httpChannel->
                      GetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                                       contentType)) ||
          contentType.IsEmpty()) {
        contentType = defaultContentType;
      }

      
      if (!charset.IsEmpty()) {
        nsCAutoString specifiedCharset;
        PRBool haveCharset;
        PRInt32 charsetStart, charsetEnd;
        rv = NS_ExtractCharsetFromContentType(contentType, specifiedCharset,
                                              &haveCharset, &charsetStart,
                                              &charsetEnd);
        if (NS_SUCCEEDED(rv)) {
          
          
          
          
          
          
          
          if (!specifiedCharset.Equals(charset,
                                       nsCaseInsensitiveCStringComparator())) {
            nsCAutoString newCharset("; charset=");
            newCharset.Append(charset);
            contentType.Replace(charsetStart, charsetEnd - charsetStart,
                                newCharset);
          }
        }
      }

      
      
      if (!NS_InputStreamIsBuffered(postDataStream)) {
        nsCOMPtr<nsIInputStream> bufferedStream;
        rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream),
                                       postDataStream, 
                                       4096);
        NS_ENSURE_SUCCESS(rv, rv);

        postDataStream = bufferedStream;
      }

      mUploadComplete = PR_FALSE;
      PRUint32 uploadTotal = 0;
      postDataStream->Available(&uploadTotal);
      mUploadTotal = uploadTotal;

      
      
      nsCOMPtr<nsIUploadChannel2> uploadChannel2(do_QueryInterface(httpChannel));
      
      NS_ASSERTION(uploadChannel2, "http must support nsIUploadChannel2");
      if (uploadChannel2) {
          uploadChannel2->ExplicitSetUploadStream(postDataStream, contentType,
                                                 -1, method, PR_FALSE);
      }
      else {
        
        
        if (contentType.IsEmpty()) {
          contentType.AssignLiteral("application/octet-stream");
        }
        nsCOMPtr<nsIUploadChannel> uploadChannel =
          do_QueryInterface(httpChannel);
        uploadChannel->SetUploadStream(postDataStream, contentType, -1);
        
        httpChannel->SetRequestMethod(method);
      }
    }
  }

  if (httpChannel) {
    nsCAutoString contentTypeHeader;
    rv = httpChannel->GetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                                       contentTypeHeader);
    if (NS_SUCCEEDED(rv)) {
      nsCAutoString contentType, charset;
      rv = NS_ParseContentType(contentTypeHeader, contentType, charset);
      NS_ENSURE_SUCCESS(rv, rv);
  
      if (!contentType.LowerCaseEqualsLiteral("text/plain") &&
          !contentType.LowerCaseEqualsLiteral("application/x-www-form-urlencoded") &&
          !contentType.LowerCaseEqualsLiteral("multipart/form-data")) {
        mCORSUnsafeHeaders.AppendElement(NS_LITERAL_CSTRING("Content-Type"));
      }
    }
  }

  
  mResponseBody.Truncate();
  mResponseBodyUnicode.SetIsVoid(PR_TRUE);
  mResponseBlob = nsnull;

  
  mResponseXML = nsnull;

  rv = CheckChannelForCrossSiteRequest(mChannel);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool withCredentials = !!(mState & XML_HTTP_REQUEST_AC_WITH_CREDENTIALS);

  
  mChannel->GetNotificationCallbacks(getter_AddRefs(mNotificationCallbacks));
  mChannel->SetNotificationCallbacks(this);

  
  nsCOMPtr<nsIStreamListener> listener = this;
  if (mState & XML_HTTP_REQUEST_MULTIPART) {
    listener = new nsMultipartProxyListener(listener);
    if (!listener) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (!IsSystemXHR()) {
    
    
    listener = new nsCORSListenerProxy(listener, mPrincipal, mChannel,
                                       withCredentials, &rv);
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  
  if ((mState & XML_HTTP_REQUEST_MULTIPART) || method.EqualsLiteral("POST")) {
    AddLoadFlags(mChannel,
        nsIRequest::LOAD_BYPASS_CACHE | nsIRequest::INHIBIT_CACHING);
  }
  
  
  
  else if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
    AddLoadFlags(mChannel,
        nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY);
  }

  
  
  
  mChannel->SetContentType(NS_LITERAL_CSTRING("application/xml"));

  
  if (mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT) {
    
    

    rv = NS_StartCORSPreflight(mChannel, listener,
                               mPrincipal, withCredentials,
                               mCORSUnsafeHeaders,
                               getter_AddRefs(mCORSPreflightChannel));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    rv = mChannel->AsyncOpen(listener, nsnull);
  }

  if (NS_FAILED(rv)) {
    
    mChannel = nsnull;
    mCORSPreflightChannel = nsnull;
    return rv;
  }

  
  if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
    mState |= XML_HTTP_REQUEST_SYNCLOOPING;

    nsCOMPtr<nsIDocument> suspendedDoc;
    nsCOMPtr<nsIRunnable> resumeTimeoutRunnable;
    if (mOwner) {
      nsCOMPtr<nsIDOMWindow> topWindow;
      if (NS_SUCCEEDED(mOwner->GetTop(getter_AddRefs(topWindow)))) {
        nsCOMPtr<nsPIDOMWindow> suspendedWindow(do_QueryInterface(topWindow));
        if (suspendedWindow &&
            (suspendedWindow = suspendedWindow->GetCurrentInnerWindow())) {
          suspendedDoc = do_QueryInterface(suspendedWindow->GetExtantDocument());
          if (suspendedDoc) {
            suspendedDoc->SuppressEventHandling();
          }
          suspendedWindow->SuspendTimeouts(1, PR_FALSE);
          resumeTimeoutRunnable = new nsResumeTimeoutsEvent(suspendedWindow);
        }
      }
    }

    ChangeState(XML_HTTP_REQUEST_SENT);
    
    
    nsIThread *thread = NS_GetCurrentThread();
    while (mState & XML_HTTP_REQUEST_SYNCLOOPING) {
      if (!NS_ProcessNextEvent(thread)) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }
    }

    if (suspendedDoc) {
      suspendedDoc->UnsuppressEventHandlingAndFireEvents(PR_TRUE);
    }

    if (resumeTimeoutRunnable) {
      NS_DispatchToCurrentThread(resumeTimeoutRunnable);
    }
  } else {
    
    
    
    
    ChangeState(XML_HTTP_REQUEST_SENT);
    if (!mUploadComplete &&
        HasListenersFor(NS_LITERAL_STRING(UPLOADPROGRESS_STR)) ||
        (mUpload && mUpload->HasListenersFor(NS_LITERAL_STRING(PROGRESS_STR)))) {
      StartProgressEventTimer();
    }
    DispatchProgressEvent(this, NS_LITERAL_STRING(LOADSTART_STR), PR_FALSE,
                          0, 0);
    if (mUpload && !mUploadComplete) {
      DispatchProgressEvent(mUpload, NS_LITERAL_STRING(LOADSTART_STR), PR_TRUE,
                            0, mUploadTotal);
    }
  }

  if (!mChannel) {
    return NS_ERROR_FAILURE;
  }

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetRequestHeader(const nsACString& header,
                                   const nsACString& value)
{
  nsresult rv;

  
  if (!IsValidHTTPToken(header)) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  if (mCORSPreflightChannel) {
    PRBool pending;
    rv = mCORSPreflightChannel->IsPending(&pending);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (pending) {
      return NS_ERROR_IN_PROGRESS;
    }
  }

  if (!mChannel)             
    return NS_ERROR_FAILURE; 

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (!httpChannel) {
    return NS_OK;
  }

  
  

  PRBool privileged;
  rv = IsCapabilityEnabled("UniversalBrowserWrite", &privileged);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (!privileged) {
    
    const char *kInvalidHeaders[] = {
      "accept-charset", "accept-encoding", "access-control-request-headers",
      "access-control-request-method", "connection", "content-length",
      "cookie", "cookie2", "content-transfer-encoding", "date", "expect",
      "host", "keep-alive", "origin", "referer", "te", "trailer",
      "transfer-encoding", "upgrade", "user-agent", "via"
    };
    PRUint32 i;
    for (i = 0; i < NS_ARRAY_LENGTH(kInvalidHeaders); ++i) {
      if (header.LowerCaseEqualsASCII(kInvalidHeaders[i])) {
        NS_WARNING("refusing to set request header");
        return NS_OK;
      }
    }
    if (StringBeginsWith(header, NS_LITERAL_CSTRING("proxy-"),
                         nsCaseInsensitiveCStringComparator()) ||
        StringBeginsWith(header, NS_LITERAL_CSTRING("sec-"),
                         nsCaseInsensitiveCStringComparator())) {
      NS_WARNING("refusing to set request header");
      return NS_OK;
    }

    
    bool safeHeader = IsSystemXHR();
    if (!safeHeader) {
      
      const char *kCrossOriginSafeHeaders[] = {
        "accept", "accept-language", "content-language", "content-type",
        "last-event-id"
      };
      for (i = 0; i < NS_ARRAY_LENGTH(kCrossOriginSafeHeaders); ++i) {
        if (header.LowerCaseEqualsASCII(kCrossOriginSafeHeaders[i])) {
          safeHeader = true;
          break;
        }
      }
    }

    if (!safeHeader) {
      mCORSUnsafeHeaders.AppendElement(header);
    }
  }

  
  return httpChannel->SetRequestHeader(header, value, PR_FALSE);
}


NS_IMETHODIMP
nsXMLHttpRequest::GetReadyState(PRUint16 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  
  if (mState & XML_HTTP_REQUEST_UNSENT) {
    *aState = UNSENT;
  } else  if (mState & (XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT)) {
    *aState = OPENED;
  } else if (mState & XML_HTTP_REQUEST_HEADERS_RECEIVED) {
    *aState = HEADERS_RECEIVED;
  } else if (mState & (XML_HTTP_REQUEST_LOADING | XML_HTTP_REQUEST_STOPPED)) {
    *aState = LOADING;
  } else if (mState & XML_HTTP_REQUEST_DONE) {
    *aState = DONE;
  } else {
    NS_ERROR("Should not happen");
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::OverrideMimeType(const nsACString& aMimeType)
{
  
  mOverrideMimeType.Assign(aMimeType);
  return NS_OK;
}



NS_IMETHODIMP
nsXMLHttpRequest::GetMultipart(PRBool *_retval)
{
  *_retval = !!(mState & XML_HTTP_REQUEST_MULTIPART);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetMultipart(PRBool aMultipart)
{
  if (!(mState & XML_HTTP_REQUEST_UNSENT)) {
    
    return NS_ERROR_IN_PROGRESS;
  }

  if (aMultipart) {
    mState |= XML_HTTP_REQUEST_MULTIPART;
  } else {
    mState &= ~XML_HTTP_REQUEST_MULTIPART;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetMozBackgroundRequest(PRBool *_retval)
{
  *_retval = !!(mState & XML_HTTP_REQUEST_BACKGROUND);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetMozBackgroundRequest(PRBool aMozBackgroundRequest)
{
  PRBool privileged;

  nsresult rv = IsCapabilityEnabled("UniversalXPConnect", &privileged);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!privileged)
    return NS_ERROR_DOM_SECURITY_ERR;

  if (!(mState & XML_HTTP_REQUEST_UNSENT)) {
    
    return NS_ERROR_IN_PROGRESS;
  }

  if (aMozBackgroundRequest) {
    mState |= XML_HTTP_REQUEST_BACKGROUND;
  } else {
    mState &= ~XML_HTTP_REQUEST_BACKGROUND;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetWithCredentials(PRBool *_retval)
{
  *_retval = !!(mState & XML_HTTP_REQUEST_AC_WITH_CREDENTIALS);

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetWithCredentials(PRBool aWithCredentials)
{
  
  if (XML_HTTP_REQUEST_SENT & mState) {
    return NS_ERROR_FAILURE;
  }
  
  if (aWithCredentials) {
    mState |= XML_HTTP_REQUEST_AC_WITH_CREDENTIALS;
  }
  else {
    mState &= ~XML_HTTP_REQUEST_AC_WITH_CREDENTIALS;
  }
  return NS_OK;
}



nsresult
nsXMLHttpRequest::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}



nsresult
nsXMLHttpRequest::Load(nsIDOMEvent* aEvent)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mState & XML_HTTP_REQUEST_STOPPED) {
    RequestCompleted();
  }
  return NS_OK;
}

nsresult
nsXMLHttpRequest::Unload(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsXMLHttpRequest::BeforeUnload(nsIDOMEvent* aEvent)
{
  return NS_OK;
}

nsresult
nsXMLHttpRequest::Abort(nsIDOMEvent* aEvent)
{
  Abort();

  return NS_OK;
}

nsresult
nsXMLHttpRequest::Error(nsIDOMEvent* aEvent)
{
  mResponseXML = nsnull;
  ChangeState(XML_HTTP_REQUEST_DONE);

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  DispatchProgressEvent(this, NS_LITERAL_STRING(ERROR_STR), PR_FALSE,
                        mResponseBody.Length(), 0);
  if (mUpload && !mUploadComplete) {
    mUploadComplete = PR_TRUE;
    DispatchProgressEvent(mUpload, NS_LITERAL_STRING(ERROR_STR), PR_TRUE,
                          mUploadTransferred, mUploadTotal);
  }

  return NS_OK;
}

nsresult
nsXMLHttpRequest::ChangeState(PRUint32 aState, PRBool aBroadcast)
{
  
  
  if (aState & XML_HTTP_REQUEST_LOADSTATES) {
    mState &= ~XML_HTTP_REQUEST_LOADSTATES;
  }
  mState |= aState;
  nsresult rv = NS_OK;

  if (mProgressNotifier &&
      !(aState & (XML_HTTP_REQUEST_HEADERS_RECEIVED | XML_HTTP_REQUEST_LOADING))) {
    mTimerIsActive = PR_FALSE;
    mProgressNotifier->Cancel();
  }

  if ((aState & XML_HTTP_REQUEST_LOADSTATES) && 
      aBroadcast &&
      (mState & XML_HTTP_REQUEST_ASYNC ||
       aState & XML_HTTP_REQUEST_OPENED ||
       aState & XML_HTTP_REQUEST_DONE)) {
    nsCOMPtr<nsIDOMEvent> event;
    rv = CreateReadystatechangeEvent(getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    DispatchDOMEvent(nsnull, event, nsnull, nsnull);
  }

  return rv;
}





class AsyncVerifyRedirectCallbackForwarder : public nsIAsyncVerifyRedirectCallback
{
public:
  AsyncVerifyRedirectCallbackForwarder(nsXMLHttpRequest *xhr)
    : mXHR(xhr)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(AsyncVerifyRedirectCallbackForwarder)

  
  NS_IMETHOD OnRedirectVerifyCallback(nsresult result)
  {
    mXHR->OnRedirectVerifyCallback(result);

    return NS_OK;
  }

private:
  nsRefPtr<nsXMLHttpRequest> mXHR;
};

NS_IMPL_CYCLE_COLLECTION_CLASS(AsyncVerifyRedirectCallbackForwarder)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(AsyncVerifyRedirectCallbackForwarder)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mXHR, nsIDOMEventListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AsyncVerifyRedirectCallbackForwarder)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mXHR)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AsyncVerifyRedirectCallbackForwarder)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIAsyncVerifyRedirectCallback)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(AsyncVerifyRedirectCallbackForwarder)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AsyncVerifyRedirectCallbackForwarder)





NS_IMETHODIMP
nsXMLHttpRequest::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                         nsIChannel *aNewChannel,
                                         PRUint32    aFlags,
                                         nsIAsyncVerifyRedirectCallback *callback)
{
  NS_PRECONDITION(aNewChannel, "Redirect without a channel?");

  nsresult rv;

  if (!NS_IsInternalSameURIRedirect(aOldChannel, aNewChannel, aFlags)) {
    rv = CheckChannelForCrossSiteRequest(aNewChannel);
    if (NS_FAILED(rv)) {
      NS_WARNING("nsXMLHttpRequest::OnChannelRedirect: "
                 "CheckChannelForCrossSiteRequest returned failure");
      return rv;
    }

    
    
    
    if ((mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT)) {
       return NS_ERROR_DOM_BAD_URI;
    }
  }

  
  mRedirectCallback = callback;
  mNewRedirectChannel = aNewChannel;

  if (mChannelEventSink) {
    nsRefPtr<AsyncVerifyRedirectCallbackForwarder> fwd =
      new AsyncVerifyRedirectCallbackForwarder(this);

    rv = mChannelEventSink->AsyncOnChannelRedirect(aOldChannel,
                                                   aNewChannel,
                                                   aFlags, fwd);
    if (NS_FAILED(rv)) {
        mRedirectCallback = nsnull;
        mNewRedirectChannel = nsnull;
    }
    return rv;
  }
  OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

void
nsXMLHttpRequest::OnRedirectVerifyCallback(nsresult result)
{
  NS_ASSERTION(mRedirectCallback, "mRedirectCallback not set in callback");
  NS_ASSERTION(mNewRedirectChannel, "mNewRedirectChannel not set in callback");

  if (NS_SUCCEEDED(result))
    mChannel = mNewRedirectChannel;
  else
    mErrorLoad = PR_TRUE;

  mNewRedirectChannel = nsnull;

  mRedirectCallback->OnRedirectVerifyCallback(result);
  mRedirectCallback = nsnull;
}





NS_IMETHODIMP
nsXMLHttpRequest::OnProgress(nsIRequest *aRequest, nsISupports *aContext, PRUint64 aProgress, PRUint64 aProgressMax)
{
  
  
  
  if (XML_HTTP_REQUEST_MPART_HEADERS & mState) {
    return NS_OK;
  }

  
  
  PRBool upload = !!((XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT) & mState);
  PRUint64 loaded = aProgress;
  PRUint64 total = aProgressMax;
  
  
  PRBool lengthComputable = (aProgressMax != LL_MAXUINT);
  if (upload) {
   if (lengthComputable) {
      PRUint64 headerSize = aProgressMax - mUploadTotal;
      loaded -= headerSize;
      total -= headerSize;
    }
    mUploadTransferred = loaded;
    mUploadProgress = aProgress;
    mUploadProgressMax = aProgressMax;
  } else {
    mLoadLengthComputable = lengthComputable;
    mLoadTotal = mLoadLengthComputable ? total : 0;
  }

  if (mTimerIsActive) {
    
    mProgressEventWasDelayed = PR_TRUE;
    return NS_OK;
  }

  if (!mErrorLoad && (mState & XML_HTTP_REQUEST_ASYNC)) {
    StartProgressEventTimer();
    NS_NAMED_LITERAL_STRING(progress, PROGRESS_STR);
    NS_NAMED_LITERAL_STRING(uploadprogress, UPLOADPROGRESS_STR);
    DispatchProgressEvent(this, upload ? uploadprogress : progress, PR_TRUE,
                          lengthComputable, loaded, lengthComputable ? total : 0,
                          aProgress, aProgressMax);

    if (upload && mUpload && !mUploadComplete) {
      NS_WARN_IF_FALSE(mUploadTotal == total, "Wrong upload total?");
      DispatchProgressEvent(mUpload, progress,  PR_TRUE, lengthComputable, loaded,
                            lengthComputable ? total : 0, aProgress, aProgressMax);
    }
  }

  if (mProgressEventSink) {
    mProgressEventSink->OnProgress(aRequest, aContext, aProgress,
                                   aProgressMax);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::OnStatus(nsIRequest *aRequest, nsISupports *aContext, nsresult aStatus, const PRUnichar *aStatusArg)
{
  if (mProgressEventSink) {
    mProgressEventSink->OnStatus(aRequest, aContext, aStatus, aStatusArg);
  }

  return NS_OK;
}

PRBool
nsXMLHttpRequest::AllowUploadProgress()
{
  return !(mState & XML_HTTP_REQUEST_USE_XSITE_AC) ||
    (mState & XML_HTTP_REQUEST_NEED_AC_PREFLIGHT);
}




NS_IMETHODIMP
nsXMLHttpRequest::GetInterface(const nsIID & aIID, void **aResult)
{
  nsresult rv;

  
  
  
  
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    mChannelEventSink = do_GetInterface(mNotificationCallbacks);
    *aResult = static_cast<nsIChannelEventSink*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIProgressEventSink))) {
    mProgressEventSink = do_GetInterface(mNotificationCallbacks);
    *aResult = static_cast<nsIProgressEventSink*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }

  
  
  if (mNotificationCallbacks) {
    rv = mNotificationCallbacks->GetInterface(aIID, aResult);
    if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(*aResult, "Lying nsIInterfaceRequestor implementation!");
      return rv;
    }
  }

  if (mState & XML_HTTP_REQUEST_BACKGROUND) {
    nsCOMPtr<nsIInterfaceRequestor> badCertHandler(do_CreateInstance(NS_BADCERTHANDLER_CONTRACTID, &rv));

    
    
    if (NS_SUCCEEDED(rv)) {
      rv = badCertHandler->GetInterface(aIID, aResult);
      if (NS_SUCCEEDED(rv))
        return rv;
    }
  }
  else if (aIID.Equals(NS_GET_IID(nsIAuthPrompt)) ||
           aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {
    nsCOMPtr<nsIPromptFactory> wwatch =
      do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    

    nsCOMPtr<nsIDOMWindow> window;
    if (mOwner) {
      window = mOwner->GetOuterWindow();
    }

    return wwatch->GetPrompt(window, aIID,
                             reinterpret_cast<void**>(aResult));

  }

  return QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
nsXMLHttpRequest::GetUpload(nsIXMLHttpRequestUpload** aUpload)
{
  *aUpload = nsnull;

  nsresult rv;
  nsIScriptContext* scriptContext =
    GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mUpload) {
    mUpload = new nsXMLHttpRequestUpload(mOwner, scriptContext);
    NS_ENSURE_TRUE(mUpload, NS_ERROR_OUT_OF_MEMORY);
  }
  NS_ADDREF(*aUpload = mUpload);
  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::Notify(nsITimer* aTimer)
{
  mTimerIsActive = PR_FALSE;
  if (NS_SUCCEEDED(CheckInnerWindowCorrectness()) && !mErrorLoad &&
      (mState & XML_HTTP_REQUEST_ASYNC)) {
    if (mProgressEventWasDelayed) {
      mProgressEventWasDelayed = PR_FALSE;
      if (!(XML_HTTP_REQUEST_MPART_HEADERS & mState)) {
        StartProgressEventTimer();
        
        
        if ((XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT) & mState) {
          DispatchProgressEvent(this, NS_LITERAL_STRING(UPLOADPROGRESS_STR),
                                PR_TRUE, PR_TRUE, mUploadTransferred,
                                mUploadTotal, mUploadProgress,
                                mUploadProgressMax);
          if (mUpload && !mUploadComplete) {
            DispatchProgressEvent(mUpload, NS_LITERAL_STRING(PROGRESS_STR),
                                  PR_TRUE, PR_TRUE, mUploadTransferred,
                                  mUploadTotal, mUploadProgress,
                                  mUploadProgressMax);
          }
        } else {
          DispatchProgressEvent(this, NS_LITERAL_STRING(PROGRESS_STR),
                                mLoadLengthComputable, mResponseBody.Length(),
                                mLoadTotal);
        }
      }
    }
  } else if (mProgressNotifier) {
    mProgressNotifier->Cancel();
  }
  return NS_OK;
}

void
nsXMLHttpRequest::StartProgressEventTimer()
{
  if (!mProgressNotifier) {
    mProgressNotifier = do_CreateInstance(NS_TIMER_CONTRACTID);
  }
  if (mProgressNotifier) {
    mProgressEventWasDelayed = PR_FALSE;
    mTimerIsActive = PR_TRUE;
    mProgressNotifier->Cancel();
    mProgressNotifier->InitWithCallback(this, NS_PROGRESS_EVENT_INTERVAL,
                                        nsITimer::TYPE_ONE_SHOT);
  }
}

NS_IMPL_ISUPPORTS1(nsXMLHttpRequest::nsHeaderVisitor, nsIHttpHeaderVisitor)

NS_IMETHODIMP nsXMLHttpRequest::
nsHeaderVisitor::VisitHeader(const nsACString &header, const nsACString &value)
{
    
    PRBool chrome = PR_FALSE; 
    IsCapabilityEnabled("UniversalXPConnect", &chrome);
    if (!chrome &&
         (header.LowerCaseEqualsASCII("set-cookie") ||
          header.LowerCaseEqualsASCII("set-cookie2"))) {
        NS_WARNING("blocked access to response header");
    } else {
        mHeaders.Append(header);
        mHeaders.Append(": ");
        mHeaders.Append(value);
        mHeaders.Append('\n');
    }
    return NS_OK;
}


nsXMLHttpProgressEvent::nsXMLHttpProgressEvent(nsIDOMProgressEvent* aInner,
                                               PRUint64 aCurrentProgress,
                                               PRUint64 aMaxProgress)
{
  mInner = static_cast<nsDOMProgressEvent*>(aInner);
  mCurProgress = aCurrentProgress;
  mMaxProgress = aMaxProgress;
}

nsXMLHttpProgressEvent::~nsXMLHttpProgressEvent()
{}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLHttpProgressEvent)

DOMCI_DATA(XMLHttpProgressEvent, nsXMLHttpProgressEvent)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXMLHttpProgressEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMProgressEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEvent, nsIDOMProgressEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEvent)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateDOMEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMProgressEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLSProgressEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XMLHttpProgressEvent)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXMLHttpProgressEvent)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXMLHttpProgressEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXMLHttpProgressEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mInner);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXMLHttpProgressEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mInner,
                                                       nsIDOMProgressEvent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP nsXMLHttpProgressEvent::GetInput(nsIDOMLSInput * *aInput)
{
  *aInput = nsnull;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXMLHttpProgressEvent::GetPosition(PRUint32 *aPosition)
{
  
  LL_L2UI(*aPosition, mCurProgress);
  return NS_OK;
}

NS_IMETHODIMP nsXMLHttpProgressEvent::GetTotalSize(PRUint32 *aTotalSize)
{
  
  LL_L2UI(*aTotalSize, mMaxProgress);
  return NS_OK;
}

