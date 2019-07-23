




































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
#include "nsThreadUtils.h"
#include "nsIUploadChannel.h"
#include "nsIDOMSerializer.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIEventListenerManager.h"
#include "nsGUIEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "prprf.h"
#include "nsIDOMEventListener.h"
#include "nsIJSContextStack.h"
#include "nsJSEnvironment.h"
#include "nsIScriptSecurityManager.h"
#include "nsWeakPtr.h"
#include "nsICharsetAlias.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"
#include "nsIVariant.h"
#include "nsVariant.h"
#include "nsIParser.h"
#include "nsLoadListenerProxy.h"
#include "nsIWindowWatcher.h"
#include "nsIAuthPrompt.h"
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
#include "nsWhitespaceTokenizer.h"
#include "nsIMultiPartChannel.h"
#include "nsIScriptObjectPrincipal.h"

#define LOAD_STR "load"
#define ERROR_STR "error"
#define PROGRESS_STR "progress"
#define UPLOADPROGRESS_STR "uploadprogress"
#define READYSTATE_STR "readystatechange"




#define XML_HTTP_REQUEST_UNINITIALIZED  (1 << 0)  // 0
#define XML_HTTP_REQUEST_OPENED         (1 << 1)  // 1 aka LOADING
#define XML_HTTP_REQUEST_LOADED         (1 << 2)  // 2
#define XML_HTTP_REQUEST_INTERACTIVE    (1 << 3)  // 3
#define XML_HTTP_REQUEST_COMPLETED      (1 << 4)  // 4
#define XML_HTTP_REQUEST_SENT           (1 << 5)  // Internal, LOADING in IE and external view
#define XML_HTTP_REQUEST_STOPPED        (1 << 6)  // Internal, INTERACTIVE in IE and external view


#define XML_HTTP_REQUEST_ABORTED        (1 << 7)  // Internal
#define XML_HTTP_REQUEST_ASYNC          (1 << 8)  // Internal
#define XML_HTTP_REQUEST_PARSEBODY      (1 << 9)  // Internal
#define XML_HTTP_REQUEST_XSITEENABLED   (1 << 10) // Internal, Is any cross-site request allowed?
                                                  
                                                  
#define XML_HTTP_REQUEST_SYNCLOOPING    (1 << 11) // Internal
#define XML_HTTP_REQUEST_MULTIPART      (1 << 12) // Internal
#define XML_HTTP_REQUEST_USE_XSITE_AC   (1 << 13) // Internal
#define XML_HTTP_REQUEST_NON_GET        (1 << 14) // Internal
#define XML_HTTP_REQUEST_GOT_FINAL_STOP (1 << 15) // Internal

#define XML_HTTP_REQUEST_LOADSTATES         \
  (XML_HTTP_REQUEST_UNINITIALIZED |         \
   XML_HTTP_REQUEST_OPENED |                \
   XML_HTTP_REQUEST_LOADED |                \
   XML_HTTP_REQUEST_INTERACTIVE |           \
   XML_HTTP_REQUEST_COMPLETED |             \
   XML_HTTP_REQUEST_SENT |                  \
   XML_HTTP_REQUEST_STOPPED)



static void AddLoadFlags(nsIRequest *request, nsLoadFlags newFlags)
{
  nsLoadFlags flags;
  request->GetLoadFlags(&flags);
  flags |= newFlags;
  request->SetLoadFlags(flags);
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



class nsACProxyListener : public nsIStreamListener,
                          public nsIInterfaceRequestor,
                          public nsIChannelEventSink
{
public:
  nsACProxyListener(nsIChannel* aOuterChannel,
                    nsIStreamListener* aOuterListener,
                    nsISupports* aOuterContext,
                    const nsACString& aRequestMethod)
   : mOuterChannel(aOuterChannel), mOuterListener(aOuterListener),
     mOuterContext(aOuterContext), mRequestMethod(aRequestMethod)
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

private:
  nsCOMPtr<nsIChannel> mOuterChannel;
  nsCOMPtr<nsIStreamListener> mOuterListener;
  nsCOMPtr<nsISupports> mOuterContext;
  nsCString mRequestMethod;
};

NS_IMPL_ISUPPORTS4(nsACProxyListener, nsIStreamListener, nsIRequestObserver,
                   nsIInterfaceRequestor, nsIChannelEventSink)

NS_IMETHODIMP
nsACProxyListener::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);

  if (NS_SUCCEEDED(rv)) {
    rv = status;
  }

  nsCOMPtr<nsIHttpChannel> http;
  if (NS_SUCCEEDED(rv)) {
    http = do_QueryInterface(aRequest, &rv);
  }
  if (NS_SUCCEEDED(rv)) {
    rv = NS_ERROR_DOM_BAD_URI;
    nsCString allow;
    http->GetResponseHeader(NS_LITERAL_CSTRING("Allow"), allow);
    nsCWhitespaceTokenizer tok(allow);
    while (tok.hasMoreTokens()) {
      if (mRequestMethod.Equals(tok.nextToken(),
                                nsCaseInsensitiveCStringComparator())) {
        rv = NS_OK;
        break;
      }
    }
  }

  if (NS_SUCCEEDED(rv)) {
    rv = mOuterChannel->AsyncOpen(mOuterListener, mOuterContext);
  }

  if (NS_FAILED(rv)) {
    mOuterChannel->Cancel(rv);
    mOuterListener->OnStartRequest(mOuterChannel, mOuterContext);
    mOuterListener->OnStopRequest(mOuterChannel, mOuterContext, rv);
    
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsACProxyListener::OnStopRequest(nsIRequest *aRequest, nsISupports *aContext,
                                 nsresult aStatus)
{
  return NS_OK;
}



NS_IMETHODIMP
nsACProxyListener::OnDataAvailable(nsIRequest *aRequest,
                                   nsISupports *ctxt,
                                   nsIInputStream *inStr,
                                   PRUint32 sourceOffset,
                                   PRUint32 count)
{
  return NS_OK;
}

NS_IMETHODIMP
nsACProxyListener::OnChannelRedirect(nsIChannel *aOldChannel,
                                     nsIChannel *aNewChannel,
                                     PRUint32 aFlags)
{
  
  return NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP
nsACProxyListener::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}








static already_AddRefed<nsIDocument>
GetDocumentFromScriptContext(nsIScriptContext *aScriptContext)
{
  if (!aScriptContext)
    return nsnull;

  nsCOMPtr<nsIDOMWindow> window =
    do_QueryInterface(aScriptContext->GetGlobalObject());
  nsIDocument *doc = nsnull;
  if (window) {
    nsCOMPtr<nsIDOMDocument> domdoc;
    window->GetDocument(getter_AddRefs(domdoc));
    if (domdoc) {
      CallQueryInterface(domdoc, &doc);
    }
  }
  return doc;
}






nsXMLHttpRequest::nsXMLHttpRequest()
  : mState(XML_HTTP_REQUEST_UNINITIALIZED)
{
  nsLayoutStatics::AddRef();
}

nsXMLHttpRequest::~nsXMLHttpRequest()
{
  if (mState & (XML_HTTP_REQUEST_STOPPED |
                XML_HTTP_REQUEST_SENT |
                XML_HTTP_REQUEST_INTERACTIVE)) {
    Abort();
  }

  NS_ABORT_IF_FALSE(!(mState & XML_HTTP_REQUEST_SYNCLOOPING), "we rather crash than hang");
  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  
  ClearEventListeners();
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

  nsIScriptContext* context = GetScriptContextFromJSContext(cx);
  if (!context) {
    return NS_OK;
  }
  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  if (secMan) {
    secMan->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  }
  NS_ENSURE_STATE(subjectPrincipal);

  mScriptContext = context;
  mPrincipal = subjectPrincipal;
  nsCOMPtr<nsPIDOMWindow> window =
    do_QueryInterface(context->GetGlobalObject());
  if (window) {
    mOwner = window->GetCurrentInnerWindow();
  }

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

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLHttpRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXMLHttpRequest)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannel)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mReadRequest)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mLoadEventListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mErrorEventListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mProgressEventListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mUploadProgressEventListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mReadystatechangeEventListeners)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnLoadListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnProgressListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnUploadProgressListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnReadystatechangeListener)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mXMLParserStreamListener)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mProgressEventSink)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOwner)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXMLHttpRequest)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChannel)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mReadRequest)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mLoadEventListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mErrorEventListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mProgressEventListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mUploadProgressEventListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mReadystatechangeEventListeners)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mScriptContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnLoadListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnProgressListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnUploadProgressListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnReadystatechangeListener)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mXMLParserStreamListener)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChannelEventSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mProgressEventSink)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOwner)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END



NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIJSXMLHttpRequest)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLoadListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIJSNativeInitializer)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XMLHttpRequest)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsXMLHttpRequest, nsIXMLHttpRequest)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsXMLHttpRequest, nsIXMLHttpRequest)




NS_IMETHODIMP
nsXMLHttpRequest::AddEventListener(const nsAString& type,
                                   nsIDOMEventListener *listener,
                                   PRBool useCapture)
{
  NS_ENSURE_ARG(listener);

  nsCOMArray<nsIDOMEventListener> *array;

#define IMPL_ADD_LISTENER(_type, _member)    \
  if (type.EqualsLiteral(_type)) {           \
    array = &(_member);                      \
  } else

  IMPL_ADD_LISTENER(LOAD_STR, mLoadEventListeners)
  IMPL_ADD_LISTENER(ERROR_STR, mErrorEventListeners)
  IMPL_ADD_LISTENER(PROGRESS_STR, mProgressEventListeners)
  IMPL_ADD_LISTENER(UPLOADPROGRESS_STR, mUploadProgressEventListeners)
  IMPL_ADD_LISTENER(READYSTATE_STR, mReadystatechangeEventListeners)
  {
    return NS_ERROR_INVALID_ARG;
  }

  array->AppendObject(listener);

#undef IMPL_ADD_LISTENER
  
  return NS_OK;
}



NS_IMETHODIMP
nsXMLHttpRequest::RemoveEventListener(const nsAString & type,
                                      nsIDOMEventListener *listener,
                                      PRBool useCapture)
{
  NS_ENSURE_ARG(listener);

  nsCOMArray<nsIDOMEventListener> *array;
#define IMPL_REMOVE_LISTENER(_type, _member)  \
  if (type.EqualsLiteral(_type)) {            \
    array = &(_member);                       \
  } else

  IMPL_REMOVE_LISTENER(LOAD_STR, mLoadEventListeners)
  IMPL_REMOVE_LISTENER(ERROR_STR, mErrorEventListeners)
  IMPL_REMOVE_LISTENER(PROGRESS_STR, mProgressEventListeners)
  IMPL_REMOVE_LISTENER(UPLOADPROGRESS_STR, mUploadProgressEventListeners)
  IMPL_REMOVE_LISTENER(READYSTATE_STR, mReadystatechangeEventListeners)
  {
    return NS_ERROR_INVALID_ARG;
  }

  
  for (PRUint32 i = array->Count() - 1; i != PRUint32(-1); --i) {
    if (array->ObjectAt(i) == listener) {
      array->RemoveObjectAt(i);
      break;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::DispatchEvent(nsIDOMEvent *evt, PRBool *_retval)
{
  

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetOnreadystatechange(nsIDOMEventListener * *aOnreadystatechange)
{
  NS_ENSURE_ARG_POINTER(aOnreadystatechange);

  NS_IF_ADDREF(*aOnreadystatechange = mOnReadystatechangeListener);

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnreadystatechange(nsIDOMEventListener * aOnreadystatechange)
{
  mOnReadystatechangeListener = aOnreadystatechange;
  return NS_OK;
}



NS_IMETHODIMP
nsXMLHttpRequest::GetOnload(nsIDOMEventListener * *aOnLoad)
{
  NS_ENSURE_ARG_POINTER(aOnLoad);

  NS_IF_ADDREF(*aOnLoad = mOnLoadListener);

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnload(nsIDOMEventListener * aOnLoad)
{
  mOnLoadListener = aOnLoad;
  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetOnerror(nsIDOMEventListener * *aOnerror)
{
  NS_ENSURE_ARG_POINTER(aOnerror);

  NS_IF_ADDREF(*aOnerror = mOnErrorListener);

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnerror(nsIDOMEventListener * aOnerror)
{
  mOnErrorListener = aOnerror;
  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetOnprogress(nsIDOMEventListener * *aOnprogress)
{
  NS_ENSURE_ARG_POINTER(aOnprogress);  

  NS_IF_ADDREF(*aOnprogress = mOnProgressListener);

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnprogress(nsIDOMEventListener * aOnprogress)
{
  mOnProgressListener = aOnprogress;
  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetOnuploadprogress(nsIDOMEventListener * *aOnuploadprogress)
{
  NS_ENSURE_ARG_POINTER(aOnuploadprogress);  

  NS_IF_ADDREF(*aOnuploadprogress = mOnUploadProgressListener);

  return NS_OK;
}

NS_IMETHODIMP
nsXMLHttpRequest::SetOnuploadprogress(nsIDOMEventListener * aOnuploadprogress)
{
  mOnUploadProgressListener = aOnuploadprogress;
  return NS_OK;
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
  if ((XML_HTTP_REQUEST_COMPLETED & mState) && mDocument) {
    *aResponseXML = mDocument;
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
  
  
  
  

  PRInt32 dataLen = mResponseBody.Length();
  if (!dataLen)
    return NS_OK;

  nsresult rv = NS_OK;

  nsCAutoString dataCharset;
  nsCOMPtr<nsIDocument> document(do_QueryInterface(mDocument));
  if (document) {
    dataCharset = document->GetDocumentCharacterSet();
  } else {
    if (NS_FAILED(DetectCharset(dataCharset)) || dataCharset.IsEmpty()) {
      
      dataCharset.AssignLiteral("UTF-8");
    }
  }

  if (dataCharset.EqualsLiteral("ASCII")) {
    CopyASCIItoUTF16(mResponseBody, aOutBuffer);

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

  PRUnichar * outBuffer =
    static_cast<PRUnichar*>(nsMemory::Alloc((outBufferLength + 1) *
                                               sizeof(PRUnichar)));
  if (!outBuffer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

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

  aOutBuffer.Assign(outBuffer, totalChars);
  nsMemory::Free(outBuffer);

  return NS_OK;
}


NS_IMETHODIMP nsXMLHttpRequest::GetResponseText(nsAString& aResponseText)
{
  nsresult rv = NS_OK;

  aResponseText.Truncate();

  if (mState & (XML_HTTP_REQUEST_COMPLETED |
                XML_HTTP_REQUEST_INTERACTIVE)) {
    rv = ConvertBodyToText(aResponseText);
  }

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetStatus(PRUint32 *aStatus)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (httpChannel) {
    nsresult rv = httpChannel->GetResponseStatus(aStatus);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      
      
      
      
      PRInt32 readyState;
      GetReadyState(&readyState);
      if (readyState >= 3) {
        *aStatus = NS_ERROR_NOT_AVAILABLE;
        return NS_OK;
      }
    }

    return rv;
  }
  *aStatus = 0;

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetStatusText(nsACString& aStatusText)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  aStatusText.Truncate();

  nsresult rv = NS_OK;

  if (httpChannel) {
    rv = httpChannel->GetResponseStatusText(aStatusText);
  }

  return rv;
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
  if (mACGetChannel) {
    mACGetChannel->Cancel(NS_BINDING_ABORTED);
  }
  mDocument = nsnull;
  mState |= XML_HTTP_REQUEST_ABORTED;

  ChangeState(XML_HTTP_REQUEST_COMPLETED, PR_TRUE, PR_TRUE);

  
  
  
  if (mState & XML_HTTP_REQUEST_ABORTED) {
    ChangeState(XML_HTTP_REQUEST_UNINITIALIZED, PR_FALSE);  
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetAllResponseHeaders(char **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    return NS_OK;
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (httpChannel) {
    nsHeaderVisitor *visitor = nsnull;
    NS_NEWXPCOM(visitor, nsHeaderVisitor);
    if (!visitor)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(visitor);

    nsresult rv = httpChannel->VisitResponseHeaders(visitor);
    if (NS_SUCCEEDED(rv))
      *_retval = ToNewCString(visitor->Headers());

    NS_RELEASE(visitor);
    return rv;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::GetResponseHeader(const nsACString& header,
                                    nsACString& _retval)
{
  nsresult rv = NS_OK;
  _retval.Truncate();

  
  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
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
      return NS_OK;
    }
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = GetCurrentHttpChannel();

  if (httpChannel) {
    rv = httpChannel->GetResponseHeader(header, _retval);
  }

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

  nsCOMPtr<nsIDocument> doc = GetDocumentFromScriptContext(mScriptContext);
  if (doc) {
    *aLoadGroup = doc->GetDocumentLoadGroup().get();  
  }

  return NS_OK;
}

nsIURI *
nsXMLHttpRequest::GetBaseURI()
{
  if (!mScriptContext) {
    return nsnull;
  }

  nsCOMPtr<nsIDocument> doc = GetDocumentFromScriptContext(mScriptContext);
  if (!doc) {
    return nsnull;
  }

  return doc->GetBaseURI();
}

nsresult
nsXMLHttpRequest::CreateEvent(const nsAString& aType, nsIDOMEvent** aDOMEvent)
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

  if (!aType.IsEmpty()) {
    (*aDOMEvent)->InitEvent(aType, PR_FALSE, PR_FALSE);
  }
  
  privevent->SetTarget(this);
  privevent->SetCurrentTarget(this);
  privevent->SetOriginalTarget(this);

  
  privevent->SetTrusted(PR_TRUE);

  return NS_OK;
}

void
nsXMLHttpRequest::CopyEventListeners(nsCOMPtr<nsIDOMEventListener>& aListener,
                                     const nsCOMArray<nsIDOMEventListener>& aListenerArray,
                                     nsCOMArray<nsIDOMEventListener>& aCopy)
{
  NS_PRECONDITION(aCopy.Count() == 0, "aCopy should start off empty");
  if (aListener)
    aCopy.AppendObject(aListener);

  aCopy.AppendObjects(aListenerArray);
}

void
nsXMLHttpRequest::NotifyEventListeners(const nsCOMArray<nsIDOMEventListener>& aListeners,
                                       nsIDOMEvent* aEvent)
{
  
  
  if (!aEvent)
    return;

  nsCOMPtr<nsIJSContextStack> stack;
  JSContext *cx = nsnull;

  if (NS_FAILED(CheckInnerWindowCorrectness())) {
    return;
  }

  if (mScriptContext) {
    stack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");

    if (stack) {
      cx = (JSContext *)mScriptContext->GetNativeContext();

      if (cx) {
        stack->Push(cx);
      }
    }
  }

  PRInt32 count = aListeners.Count();
  for (PRInt32 index = 0; index < count; ++index) {
    nsIDOMEventListener* listener = aListeners[index];
    
    if (listener) {
      listener->HandleEvent(aEvent);
    }
  }

  if (cx) {
    stack->Pop(&cx);
  }
}

void
nsXMLHttpRequest::ClearEventListeners()
{
  
  
  
  
  
  
  

  mLoadEventListeners.Clear();
  mErrorEventListeners.Clear();
  mProgressEventListeners.Clear();
  mUploadProgressEventListeners.Clear();
  mReadystatechangeEventListeners.Clear();

  mOnLoadListener = nsnull;
  mOnErrorListener = nsnull;
  mOnProgressListener = nsnull;
  mOnUploadProgressListener = nsnull;
  mOnReadystatechangeListener = nsnull;
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

static PRBool
IsSameOrigin(nsIPrincipal* aPrincipal, nsIChannel* aChannel)
{
  if (!aPrincipal) {
    
    
    return PR_TRUE;
  }

  nsCOMPtr<nsIURI> codebase;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(codebase));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> channelURI;
  rv = aChannel->GetURI(getter_AddRefs(channelURI));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = nsContentUtils::GetSecurityManager()->
    CheckSameOriginURI(codebase, channelURI, PR_FALSE);
  return NS_SUCCEEDED(rv);
}

nsresult
nsXMLHttpRequest::CheckChannelForCrossSiteRequest()
{
  
  
  if ((mState & XML_HTTP_REQUEST_XSITEENABLED) ||
      IsSameOrigin(mPrincipal, mChannel)) {
    return NS_OK;
  }

  

  
  mState |= XML_HTTP_REQUEST_USE_XSITE_AC;

  
  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(mChannel);
  if (http) {
    PRUint32 i;
    for (i = 0; i < mExtraRequestHeaders.Length(); ++i) {
      http->SetRequestHeader(mExtraRequestHeaders[i], EmptyCString(), PR_FALSE);
    }
    mExtraRequestHeaders.Clear();
  }

  
  
  nsCOMPtr<nsIURI> channelURI;
  nsresult rv = mChannel->GetURI(getter_AddRefs(channelURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString userpass;
  channelURI->GetUserPass(userpass);
  return userpass.IsEmpty() ? NS_OK : NS_ERROR_DOM_BAD_URI;
}


NS_IMETHODIMP
nsXMLHttpRequest::OpenRequest(const nsACString& method,
                              const nsACString& url,
                              PRBool async,
                              const nsAString& user,
                              const nsAString& password)
{
  NS_ENSURE_ARG(!method.IsEmpty());
  NS_ENSURE_ARG(!url.IsEmpty());

  
  
  if (method.LowerCaseEqualsLiteral("trace") ||
      method.LowerCaseEqualsLiteral("track")) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  PRBool authp = PR_FALSE;

  if (mState & (XML_HTTP_REQUEST_OPENED |
                XML_HTTP_REQUEST_LOADED |
                XML_HTTP_REQUEST_INTERACTIVE |
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

  rv = NS_NewURI(getter_AddRefs(uri), url, nsnull, GetBaseURI());
  if (NS_FAILED(rv)) return rv;

  
  
  rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocument> doc = GetDocumentFromScriptContext(mScriptContext);
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

  
  
  
  
  nsLoadFlags loadFlags;
  if (mOnProgressListener ||
      mOnUploadProgressListener ||
      mProgressEventListeners.Count() != 0 ||
      mUploadProgressEventListeners.Count() != 0) {
    loadFlags = nsIRequest::LOAD_NORMAL;
  } else {
    loadFlags = nsIRequest::LOAD_BACKGROUND;
  }
  rv = NS_NewChannel(getter_AddRefs(mChannel), uri, nsnull, loadGroup, nsnull,
                     loadFlags);
  if (NS_FAILED(rv)) return rv;

  
  if (!(mState & XML_HTTP_REQUEST_XSITEENABLED) &&
      !IsSameOrigin(mPrincipal, mChannel)) {
    mState |= XML_HTTP_REQUEST_USE_XSITE_AC;
  }

  

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (httpChannel) {
    rv = httpChannel->SetRequestMethod(method);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!method.LowerCaseEqualsLiteral("get")) {
      mState |= XML_HTTP_REQUEST_NON_GET;
    }
  }

  
  
  if ((mState & XML_HTTP_REQUEST_USE_XSITE_AC) &&
      (mState & XML_HTTP_REQUEST_NON_GET)) {
    rv = NS_NewChannel(getter_AddRefs(mACGetChannel), uri, nsnull, loadGroup, nsnull,
                       loadFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIHttpChannel> acHttp = do_QueryInterface(mACGetChannel);
    rv = acHttp->SetRequestHeader(
      NS_LITERAL_CSTRING("XMLHttpRequest-Security-Check"), method, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  ChangeState(XML_HTTP_REQUEST_OPENED);

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::Open(const nsACString& method, const nsACString& url)
{
  nsresult rv = NS_OK;
  PRBool async = PR_TRUE;
  nsAutoString user, password;

  nsAXPCNativeCallContext *cc = nsnull;
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  if (xpc) {
    rv = xpc->GetCurrentNativeCallContext(&cc);
  }

  if (NS_SUCCEEDED(rv) && cc) {
    PRUint32 argc;
    rv = cc->GetArgc(&argc);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    jsval* argv;
    rv = cc->GetArgvPtr(&argv);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    JSContext* cx;
    rv = cc->GetJSContext(&cx);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIURI> targetURI;
    rv = NS_NewURI(getter_AddRefs(targetURI), url, nsnull, GetBaseURI());
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
    if (!secMan) {
      return NS_ERROR_FAILURE;
    }

    rv = secMan->CheckConnect(cx, targetURI, "XMLHttpRequest", "open-uri");
    if (NS_FAILED(rv))
    {
      
      return NS_OK;
    }

    
    
    PRBool crossSiteAccessEnabled;
    rv = secMan->IsCapabilityEnabled("UniversalBrowserRead",
                                     &crossSiteAccessEnabled);
    if (NS_FAILED(rv)) return rv;
    if (crossSiteAccessEnabled) {
      mState |= XML_HTTP_REQUEST_XSITEENABLED;
    } else {
      mState &= ~XML_HTTP_REQUEST_XSITEENABLED;
    }

    if (argc > 2) {
      JSAutoRequest ar(cx);
      JSBool asyncBool;
      ::JS_ValueToBoolean(cx, argv[2], &asyncBool);
      async = (PRBool)asyncBool;

      if (argc > 3 && !JSVAL_IS_NULL(argv[3]) && !JSVAL_IS_VOID(argv[3])) {
        JSString* userStr = ::JS_ValueToString(cx, argv[3]);

        if (userStr) {
          user.Assign(reinterpret_cast<PRUnichar *>
                                      (::JS_GetStringChars(userStr)),
                      ::JS_GetStringLength(userStr));
        }

        if (argc > 4 && !JSVAL_IS_NULL(argv[4]) && !JSVAL_IS_VOID(argv[4])) {
          JSString* passwdStr = JS_ValueToString(cx, argv[4]);

          if (passwdStr) {
            password.Assign(reinterpret_cast<PRUnichar *>
                                            (::JS_GetStringChars(passwdStr)),
                            ::JS_GetStringLength(passwdStr));
          }
        }
      }
    }
  }

  return OpenRequest(method, url, async, user, password);
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

  
  xmlHttpRequest->mResponseBody.Append(fromRawSegment,count);

  nsresult rv = NS_OK;

  if (xmlHttpRequest->mState & XML_HTTP_REQUEST_PARSEBODY) {
    

    
    
    
    nsCOMPtr<nsIInputStream> copyStream;
    rv = NS_NewByteInputStream(getter_AddRefs(copyStream), fromRawSegment, count);

    if (NS_SUCCEEDED(rv)) {
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

  xmlHttpRequest->ChangeState(XML_HTTP_REQUEST_INTERACTIVE);

  if (NS_SUCCEEDED(rv)) {
    *writeCount = count;
  } else {
    *writeCount = 0;
  }

  return rv;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnDataAvailable(nsIRequest *request, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  NS_ENSURE_ARG_POINTER(inStr);

  NS_ABORT_IF_FALSE(mContext.get() == ctxt,"start context different from OnDataAvailable context");

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
  if (!IsSameOrBaseChannel(request, mChannel)) {
    return NS_OK;
  }

  
  if (mState & XML_HTTP_REQUEST_UNINITIALIZED)
    return NS_OK;

  if (mState & XML_HTTP_REQUEST_ABORTED) {
    NS_ERROR("Ugh, still getting data on an aborted XMLHttpRequest!");

    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  channel->SetOwner(mPrincipal);

  mReadRequest = request;
  mContext = ctxt;
  mState |= XML_HTTP_REQUEST_PARSEBODY;
  ChangeState(XML_HTTP_REQUEST_LOADED);

  nsIURI* uri = GetBaseURI();

  
  const nsAString& emptyStr = EmptyString();
  nsCOMPtr<nsIScriptGlobalObject> global = do_QueryInterface(mOwner);
  nsresult rv = nsContentUtils::CreateDocument(emptyStr, emptyStr, nsnull, uri,
                                               uri, mPrincipal, global,
                                               getter_AddRefs(mDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
    if (htmlDoc) {
      htmlDoc->DisableCookieAccess();
    }
  }

  
  mResponseBody.Truncate();

  
  nsCOMPtr<nsPIDOMEventTarget> target(do_QueryInterface(mDocument));
  if (target) {
    nsWeakPtr requestWeak =
      do_GetWeakReference(static_cast<nsIXMLHttpRequest*>(this));
    nsCOMPtr<nsIDOMEventListener> proxy = new nsLoadListenerProxy(requestWeak);
    if (!proxy) return NS_ERROR_OUT_OF_MEMORY;

    
    rv = target->AddEventListenerByIID(static_cast<nsIDOMEventListener*>
                                                  (proxy),
                                       NS_GET_IID(nsIDOMLoadListener));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  }

  nsresult status;
  request->GetStatus(&status);

  PRBool parseBody = PR_TRUE;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));
  if (httpChannel) {
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
    nsCOMPtr<nsIStreamListener> listener;
    nsCOMPtr<nsILoadGroup> loadGroup;
    channel->GetLoadGroup(getter_AddRefs(loadGroup));

    nsCOMPtr<nsIDocument> document(do_QueryInterface(mDocument));
    if (!document) {
      return NS_ERROR_FAILURE;
    }

    rv = document->StartDocumentLoad(kLoadAsData, channel, loadGroup, nsnull,
                                     getter_AddRefs(listener), PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    mXMLParserStreamListener = listener;
    return mXMLParserStreamListener->OnStartRequest(request, ctxt);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
  if (!IsSameOrBaseChannel(request, mChannel)) {
    return NS_OK;
  }

  
  if (mState & XML_HTTP_REQUEST_UNINITIALIZED)
    return NS_OK;

  nsresult rv = NS_OK;

  nsCOMPtr<nsIParser> parser;

  
  
  
  
  
  

  
  if (mState & XML_HTTP_REQUEST_PARSEBODY && mXMLParserStreamListener) {
    parser = do_QueryInterface(mXMLParserStreamListener);
    NS_ABORT_IF_FALSE(parser, "stream listener was expected to be a parser");
    rv = mXMLParserStreamListener->OnStopRequest(request, ctxt, status);
  }

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

  mXMLParserStreamListener = nsnull;
  mReadRequest = nsnull;
  mContext = nsnull;

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

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

  if (mScriptContext) {
    
    
    
    mScriptContext->GC();
  }

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  return rv;
}

nsresult
nsXMLHttpRequest::RequestCompleted()
{
  nsresult rv = NS_OK;

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  
  
  
  if (mState & (XML_HTTP_REQUEST_UNINITIALIZED |
                XML_HTTP_REQUEST_COMPLETED)) {
    return NS_OK;
  }

  
  
  nsCOMArray<nsIDOMEventListener> loadEventListeners;
  CopyEventListeners(mOnLoadListener, mLoadEventListeners, loadEventListeners);

  
  nsCOMPtr<nsIDOMEvent> domevent;
  if (loadEventListeners.Count()) {
    rv = CreateEvent(NS_LITERAL_STRING(LOAD_STR), getter_AddRefs(domevent));
  }

  
  
  
  
  if (mDocument) {
    nsCOMPtr<nsIDOMElement> root;
    mDocument->GetDocumentElement(getter_AddRefs(root));
    if (!root) {
      mDocument = nsnull;
    }
  }

  
  ChangeState(XML_HTTP_REQUEST_COMPLETED, PR_TRUE,
              !!(mState & XML_HTTP_REQUEST_GOT_FINAL_STOP));

  if (NS_SUCCEEDED(rv) && domevent) {
    NotifyEventListeners(loadEventListeners, domevent);
  }

  if (!(mState & XML_HTTP_REQUEST_GOT_FINAL_STOP)) {
    
    ChangeState(XML_HTTP_REQUEST_OPENED);
  }

  nsJSContext::MaybeCC(PR_FALSE);
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


NS_IMETHODIMP
nsXMLHttpRequest::Send(nsIVariant *aBody)
{
  nsresult rv = CheckInnerWindowCorrectness();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (XML_HTTP_REQUEST_SENT & mState) {
    return NS_ERROR_FAILURE;
  }

  
  if (!mChannel || !(XML_HTTP_REQUEST_OPENED & mState)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  
  

  
  
  nsCAutoString method;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(mChannel));

  if (httpChannel) {
    httpChannel->GetRequestMethod(method); 

    if (mPrincipal) {
      nsCOMPtr<nsIURI> codebase;
      mPrincipal->GetURI(getter_AddRefs(codebase));

      httpChannel->SetReferrer(codebase);
    }
  }

  if (aBody && httpChannel && !method.EqualsLiteral("GET")) {
    nsXPIDLString serial;
    nsCOMPtr<nsIInputStream> postDataStream;
    nsCAutoString charset(NS_LITERAL_CSTRING("UTF-8"));

    PRUint16 dataType;
    rv = aBody->GetDataType(&dataType);
    if (NS_FAILED(rv))
      return rv;

    switch (dataType) {
    case nsIDataType::VTYPE_INTERFACE:
    case nsIDataType::VTYPE_INTERFACE_IS:
      {
        nsCOMPtr<nsISupports> supports;
        nsID *iid;
        rv = aBody->GetAsInterface(&iid, getter_AddRefs(supports));
        if (NS_FAILED(rv))
          return rv;
        if (iid)
          nsMemory::Free(iid);

        
        nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(supports));
        if (doc) {
          nsCOMPtr<nsIDOMSerializer> serializer(do_CreateInstance(NS_XMLSERIALIZER_CONTRACTID, &rv));
          if (NS_FAILED(rv)) return rv;

          nsCOMPtr<nsIDocument> baseDoc(do_QueryInterface(doc));
          if (baseDoc) {
            charset = baseDoc->GetDocumentCharacterSet();
          }

          
          
          nsCOMPtr<nsIInputStream> input;
          nsCOMPtr<nsIOutputStream> output;
          rv = NS_NewPipe(getter_AddRefs(input), getter_AddRefs(output),
                          0, PR_UINT32_MAX);
          NS_ENSURE_SUCCESS(rv, rv);

          
          
          rv = serializer->SerializeToStream(doc, output, EmptyCString());
          NS_ENSURE_SUCCESS(rv, rv);

          output->Close();
          postDataStream = input;
        } else {
          
          nsCOMPtr<nsISupportsString> wstr(do_QueryInterface(supports));
          if (wstr) {
            wstr->GetData(serial);
          } else {
            
            nsCOMPtr<nsIInputStream> stream(do_QueryInterface(supports));
            if (stream) {
              postDataStream = stream;
              charset.Truncate();
            }
          }
        }
      }
      break;
    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_EMPTY:
      
      break;
    case nsIDataType::VTYPE_EMPTY_ARRAY:
    case nsIDataType::VTYPE_ARRAY:
      
      return NS_ERROR_INVALID_ARG;
    default:
      
      rv = aBody->GetAsWString(getter_Copies(serial));
      if (NS_FAILED(rv))
        return rv;
      break;
    }

    if (serial) {
      
      nsCOMPtr<nsIScriptableUnicodeConverter> converter =
        do_CreateInstance("@mozilla.org/intl/scriptableunicodeconverter", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = converter->SetCharset("UTF-8");
      NS_ENSURE_SUCCESS(rv, rv);

      rv = converter->ConvertToInputStream(serial,
                                           getter_AddRefs(postDataStream));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (postDataStream) {
      nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
      NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");

      
      
      nsCAutoString contentType;
      if (NS_FAILED(httpChannel->
                      GetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                                       contentType)) ||
          contentType.IsEmpty()) {
        contentType = NS_LITERAL_CSTRING("application/xml");
      }

      
      if (!charset.IsEmpty()) {
        nsCAutoString specifiedCharset;
        PRBool haveCharset;
        PRInt32 charsetStart, charsetEnd;
        rv = NS_ExtractCharsetFromContentType(contentType, specifiedCharset,
                                              &haveCharset, &charsetStart,
                                              &charsetEnd);
        if (NS_FAILED(rv)) {
          contentType.AssignLiteral("application/xml");
          specifiedCharset.Truncate();
          haveCharset = PR_FALSE;
        }

        if (!haveCharset) {
          charsetStart = charsetEnd = contentType.Length();
        } 

        
        
        
        
        
        
        
        if (!specifiedCharset.Equals(charset,
                                     nsCaseInsensitiveCStringComparator())) {
          nsCAutoString newCharset("; charset=");
          newCharset.Append(charset);
          contentType.Replace(charsetStart, charsetEnd - charsetStart,
                              newCharset);
        }
      }

      rv = uploadChannel->SetUploadStream(postDataStream, contentType, -1);
      
      if (httpChannel) {
        httpChannel->SetRequestMethod(method);
      }
    }
  }

  
  mResponseBody.Truncate();

  
  mDocument = nsnull;

  if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
    mState |= XML_HTTP_REQUEST_SYNCLOOPING;
  }

  rv = CheckChannelForCrossSiteRequest();
  NS_ENSURE_SUCCESS(rv, rv);

  
  mChannel->GetNotificationCallbacks(getter_AddRefs(mNotificationCallbacks));
  mChannel->SetNotificationCallbacks(this);

  
  nsCOMPtr<nsIStreamListener> listener = this;
  if (mState & XML_HTTP_REQUEST_MULTIPART) {
    listener = new nsMultipartProxyListener(listener);
    if (!listener) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (!(mState & XML_HTTP_REQUEST_XSITEENABLED)) {
    
    
    listener = new nsCrossSiteListenerProxy(listener, mPrincipal);
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
  }

  
  
  
  
  
  
  if ((mState & XML_HTTP_REQUEST_MULTIPART) || method.EqualsLiteral("POST")) {
    AddLoadFlags(mChannel,
        nsIRequest::LOAD_BYPASS_CACHE | nsIRequest::INHIBIT_CACHING);
  }
  
  
  
  else if (mState & XML_HTTP_REQUEST_SYNCLOOPING) {
    AddLoadFlags(mChannel,
        nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY);
    if (mACGetChannel) {
      AddLoadFlags(mACGetChannel,
          nsICachingChannel::LOAD_BYPASS_LOCAL_CACHE_IF_BUSY);
    }
  }

  
  
  
  mChannel->SetContentType(NS_LITERAL_CSTRING("application/xml"));

  
  
  if (mACGetChannel) {
    nsCOMPtr<nsIStreamListener> acListener =
      new nsACProxyListener(mChannel, listener, nsnull, method);
    NS_ENSURE_TRUE(acListener, NS_ERROR_OUT_OF_MEMORY);

    listener = new nsCrossSiteListenerProxy(acListener, mPrincipal);
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

    rv = mACGetChannel->AsyncOpen(listener, nsnull);
  }
  else {
    
    rv = mChannel->AsyncOpen(listener, nsnull);
  }

  if (NS_FAILED(rv)) {
    
    mChannel = nsnull;
    return rv;
  }

  
  
  
  
  ChangeState(XML_HTTP_REQUEST_SENT);

  
  if (!(mState & XML_HTTP_REQUEST_ASYNC)) {
    nsIThread *thread = NS_GetCurrentThread();
    while (mState & XML_HTTP_REQUEST_SYNCLOOPING) {
      if (!NS_ProcessNextEvent(thread)) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }
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

  
  
  
  if (mACGetChannel) {
    PRBool pending;
    rv = mACGetChannel->IsPending(&pending);
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

  
  

  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
  if (!secMan) {
    return NS_ERROR_FAILURE;
  }

  PRBool privileged;
  rv = secMan->IsCapabilityEnabled("UniversalBrowserWrite", &privileged);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (!privileged) {
    
    const char *kInvalidHeaders[] = {
      "accept-charset", "accept-encoding", "connection", "content-length",
      "content-transfer-encoding", "date", "expect", "host", "keep-alive",
      "proxy-connection", "referer", "referer-root", "te", "trailer",
      "transfer-encoding", "upgrade", "via", "xmlhttprequest-security-check"
    };
    PRUint32 i;
    for (i = 0; i < NS_ARRAY_LENGTH(kInvalidHeaders); ++i) {
      if (header.LowerCaseEqualsASCII(kInvalidHeaders[i])) {
        NS_WARNING("refusing to set request header");
        return NS_OK;
      }
    }

    
    PRBool safeHeader = !!(mState & XML_HTTP_REQUEST_XSITEENABLED);
    if (!safeHeader) {
      const char *kCrossOriginSafeHeaders[] = {
        "accept", "accept-language"
      };
      for (i = 0; i < NS_ARRAY_LENGTH(kCrossOriginSafeHeaders); ++i) {
        if (header.LowerCaseEqualsASCII(kCrossOriginSafeHeaders[i])) {
          safeHeader = PR_TRUE;
          break;
        }
      }
    }

    if (!safeHeader) {
      
      
      if (mState & XML_HTTP_REQUEST_USE_XSITE_AC) {
        return NS_ERROR_FAILURE;
      }

      
      
      mExtraRequestHeaders.AppendElement(header);
    }
  }

  
  return httpChannel->SetRequestHeader(header, value, PR_FALSE);
}


NS_IMETHODIMP
nsXMLHttpRequest::GetReadyState(PRInt32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  
  if (mState & XML_HTTP_REQUEST_UNINITIALIZED) {
    *aState = 0; 
  } else  if (mState & (XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT)) {
    *aState = 1; 
  } else if (mState & XML_HTTP_REQUEST_LOADED) {
    *aState = 2; 
  } else if (mState & (XML_HTTP_REQUEST_INTERACTIVE | XML_HTTP_REQUEST_STOPPED)) {
    *aState = 3; 
  } else if (mState & XML_HTTP_REQUEST_COMPLETED) {
    *aState = 4; 
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
  *_retval = mState & XML_HTTP_REQUEST_MULTIPART;

  return NS_OK;
}


NS_IMETHODIMP
nsXMLHttpRequest::SetMultipart(PRBool aMultipart)
{
  if (!(mState & XML_HTTP_REQUEST_UNINITIALIZED)) {
    
    return NS_ERROR_IN_PROGRESS;
  }

  if (aMultipart) {
    mState |= XML_HTTP_REQUEST_MULTIPART;
  } else {
    mState &= ~XML_HTTP_REQUEST_MULTIPART;
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

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  return NS_OK;
}

nsresult
nsXMLHttpRequest::Error(nsIDOMEvent* aEvent)
{
  nsCOMArray<nsIDOMEventListener> errorEventListeners;
  CopyEventListeners(mOnErrorListener, mErrorEventListeners,
                     errorEventListeners);

  
  nsCOMPtr<nsIDOMEvent> event = aEvent;
  if (!event && errorEventListeners.Count()) {
    CreateEvent(NS_LITERAL_STRING(ERROR_STR), getter_AddRefs(event));
  }

  mDocument = nsnull;
  ChangeState(XML_HTTP_REQUEST_COMPLETED);

  mState &= ~XML_HTTP_REQUEST_SYNCLOOPING;

  ClearEventListeners();
  
  if (event) {
    NotifyEventListeners(errorEventListeners, event);
  }

  nsJSContext::MaybeCC(PR_FALSE);
  return NS_OK;
}

nsresult
nsXMLHttpRequest::ChangeState(PRUint32 aState, PRBool aBroadcast,
                              PRBool aClearEventListeners)
{
  
  
  if (aState & XML_HTTP_REQUEST_LOADSTATES) {
    mState &= ~XML_HTTP_REQUEST_LOADSTATES;
  }
  mState |= aState;
  nsresult rv = NS_OK;

  
  nsCOMArray<nsIDOMEventListener> readystatechangeEventListeners;
  CopyEventListeners(mOnReadystatechangeListener,
                     mReadystatechangeEventListeners,
                     readystatechangeEventListeners);

  if (aClearEventListeners) {
    ClearEventListeners();
  }

  if ((mState & XML_HTTP_REQUEST_ASYNC) &&
      (aState & XML_HTTP_REQUEST_LOADSTATES) && 
      aBroadcast &&
      readystatechangeEventListeners.Count()) {
    nsCOMPtr<nsIDOMEvent> event;
    rv = CreateEvent(NS_LITERAL_STRING(READYSTATE_STR),
                     getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);

    NotifyEventListeners(readystatechangeEventListeners, event);
  }

  return rv;
}




NS_IMETHODIMP
nsXMLHttpRequest::OnChannelRedirect(nsIChannel *aOldChannel,
                                    nsIChannel *aNewChannel,
                                    PRUint32    aFlags)
{
  NS_PRECONDITION(aNewChannel, "Redirect without a channel?");

  nsresult rv;
  if (mChannelEventSink) {
    rv =
      mChannelEventSink->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mChannel = aNewChannel;

  rv = CheckChannelForCrossSiteRequest();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if ((mState & XML_HTTP_REQUEST_NON_GET) &&
      (mState & XML_HTTP_REQUEST_USE_XSITE_AC)) {
    return NS_ERROR_DOM_BAD_URI;
  }

  return NS_OK;
}





NS_IMETHODIMP
nsXMLHttpRequest::OnProgress(nsIRequest *aRequest, nsISupports *aContext, PRUint64 aProgress, PRUint64 aProgressMax)
{
  
  
  PRBool downloading =
    !((XML_HTTP_REQUEST_OPENED | XML_HTTP_REQUEST_SENT) & mState);
  nsCOMArray<nsIDOMEventListener> progressListeners;
  if (downloading) {
    CopyEventListeners(mOnProgressListener,
                       mProgressEventListeners, progressListeners);
  } else {
    CopyEventListeners(mOnUploadProgressListener,
                       mUploadProgressEventListeners, progressListeners);
  }
  
  if (progressListeners.Count()) {
    nsCOMPtr<nsIDOMEvent> event;
    nsresult rv = CreateEvent(NS_LITERAL_STRING(PROGRESS_STR),
                              getter_AddRefs(event));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsXMLHttpProgressEvent * progressEvent =
      new nsXMLHttpProgressEvent(event, aProgress, aProgressMax); 
    if (!progressEvent)
      return NS_ERROR_OUT_OF_MEMORY;

    event = progressEvent;
    NotifyEventListeners(progressListeners, event);
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




NS_IMETHODIMP
nsXMLHttpRequest::GetInterface(const nsIID & aIID, void **aResult)
{
  
  
  
  
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
    nsresult rv = mNotificationCallbacks->GetInterface(aIID, aResult);
    if (NS_SUCCEEDED(rv)) {
      NS_ASSERTION(*aResult, "Lying nsIInterfaceRequestor implementation!");
      return rv;
    }
  }
  
  if (aIID.Equals(NS_GET_IID(nsIAuthPrompt))) {    
    *aResult = nsnull;

    nsresult rv;
    nsCOMPtr<nsIWindowWatcher> ww(do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIAuthPrompt> prompt;
    rv = ww->GetNewAuthPrompter(nsnull, getter_AddRefs(prompt));
    if (NS_FAILED(rv))
      return rv;

    nsIAuthPrompt *p = prompt.get();
    NS_ADDREF(p);
    *aResult = p;
    return NS_OK;
  }

  return QueryInterface(aIID, aResult);
}


NS_IMPL_ISUPPORTS1(nsXMLHttpRequest::nsHeaderVisitor, nsIHttpHeaderVisitor)

NS_IMETHODIMP nsXMLHttpRequest::
nsHeaderVisitor::VisitHeader(const nsACString &header, const nsACString &value)
{
    mHeaders.Append(header);
    mHeaders.Append(": ");
    mHeaders.Append(value);
    mHeaders.Append('\n');
    return NS_OK;
}


nsXMLHttpProgressEvent::nsXMLHttpProgressEvent(nsIDOMEvent * aInner, PRUint64 aCurrentProgress, PRUint64 aMaxProgress)
{
  mInner = aInner; 
  mCurProgress = aCurrentProgress;
  mMaxProgress = aMaxProgress;
}

nsXMLHttpProgressEvent::~nsXMLHttpProgressEvent()
{}


NS_INTERFACE_MAP_BEGIN(nsXMLHttpProgressEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMLSProgressEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLSProgressEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XMLHttpProgressEvent)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsXMLHttpProgressEvent)
NS_IMPL_RELEASE(nsXMLHttpProgressEvent)

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
